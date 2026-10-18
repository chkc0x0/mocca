#include "utils/ScopedProfiler.h"
#ifdef MC_PROFILER

#	include <algorithm>
#	include <atomic>
#	include <chrono>
#	include <cstdio>
#	include <cstdlib>
#	include <cstring>
#	include <memory>
#	include <unordered_map>
#	include <vector>

namespace mocca::detail
{
	struct Node
	{
		const Profiler::Slot* Slot;
		Node* Parent;
		Node* FirstChild = nullptr;
		Node* NextSibling = nullptr;
		Node* Hint = nullptr;
		uint64_t Calls = 0;
		uint64_t Total = 0;
		uint64_t Self = 0;
		uint64_t Max = 0;
		uint64_t Pending = 0;

		Node(const Profiler::Slot* s, Node* p) : Slot(s), Parent(p)
		{
		}

		auto Child(const Profiler::Slot* s) -> Node*
		{
			if ((Hint != nullptr) && Hint->Slot == s)
			{
				return Hint;
			}
			for (Node* c = FirstChild; c != nullptr; c = c->NextSibling)
			{
				if (c->Slot == s)
				{
					return Hint = c;
				}
			}
			Node* n = new Node(s, this);
			n->NextSibling = FirstChild;
			FirstChild = n;
			return Hint = n;
		}
	};

	struct ThreadState
	{
		Node Root{nullptr, nullptr};
		Node* Cur = &Root;
		ThreadState* Next = nullptr;

		ThreadState();
	};

	std::atomic<ThreadState*> threads{nullptr};
	std::atomic<uint64_t> Frames{0};
	thread_local ThreadState* state = nullptr;

	const bool Enabled = getenv("MC_PROFILE") != nullptr;

	auto nowNs() -> uint64_t
	{
		using namespace std::chrono;
		return static_cast<uint64_t>(
			duration_cast<nanoseconds>(steady_clock::now().time_since_epoch())
				.count()
		);
	}

	auto threadState() -> ThreadState&
	{
		if (state == nullptr)
		{
			state = new struct ThreadState();
		}
		return *state;
	}

	ThreadState::ThreadState()
	{
		Next = threads.load(std::memory_order_relaxed);
		while (!threads.compare_exchange_weak(
			Next,
			this,
			std::memory_order_release,
			std::memory_order_relaxed
		))
		{
		}
	}

	struct MNode
	{
		const Profiler::Slot* Slot = nullptr;
		uint64_t Calls = 0;
		uint64_t Total = 0;
		uint64_t Self = 0;
		uint64_t Max = 0;
		std::vector<std::unique_ptr<MNode>> Kids;
	};

	void mergeInto(MNode& dst, const Node& src)
	{
		for (const Node* c = src.FirstChild; c != nullptr; c = c->NextSibling)
		{
			MNode* m = nullptr;
			for (auto& k : dst.Kids)
			{
				if (k->Slot == c->Slot)
				{
					m = k.get();
					break;
				}
			}
			if (m == nullptr)
			{
				dst.Kids.push_back(std::make_unique<MNode>());
				m = dst.Kids.back().get();
				m->Slot = c->Slot;
			}
			m->Calls += c->Calls;
			m->Total += c->Total;
			m->Self += c->Self;
			m->Max = std::max(m->Max, c->Max);
			mergeInto(*m, *c);
		}
	}

	void sortKids(MNode& n)
	{
		std::sort(
			n.Kids.begin(),
			n.Kids.end(),
			[](const auto& a, const auto& b) -> auto
			{ return a->Total > b->Total; }
		);
		for (auto& k : n.Kids)
		{
			sortKids(*k);
		}
	}

	struct FlatRow
	{
		const Profiler::Slot* Slot = nullptr;
		uint64_t Calls = 0;
		uint64_t Outer = 0;
		uint64_t Total = 0;
		uint64_t Self = 0;
		uint64_t Max = 0;
	};

	void collectFlat(
		const MNode& n,
		std::vector<const Profiler::Slot*>& path,
		std::unordered_map<const Profiler::Slot*, FlatRow>& out
	)
	{
		for (const auto& k : n.Kids)
		{
			FlatRow& r = out[k->Slot];
			r.Slot = k->Slot;
			r.Calls += k->Calls;
			r.Self += k->Self;
			r.Max = std::max(r.Max, k->Max);
			if (std::find(path.begin(), path.end(), k->Slot) == path.end())
			{
				r.Total += k->Total;
				r.Outer += k->Calls;
			}
			path.push_back(k->Slot);
			collectFlat(*k, path, out);
			path.pop_back();
		}
	}

	void printRow(
		double firstMs,
		double secondMs,
		uint64_t calls,
		double avgUs,
		double maxUs,
		uint64_t frames,
		double perFrameMs,
		int indent,
		const Profiler::Slot& s
	)
	{
		fprintf(
			stderr,
			"%10.3f %10.3f %10llu %9.2f %9.2f ",
			firstMs,
			secondMs,
			static_cast<unsigned long long>(calls),
			avgUs,
			maxUs
		);
		if (frames != 0U)
		{
			fprintf(stderr, "%9.4f", perFrameMs);
		}
		else
		{
			fprintf(stderr, "%9s", "-");
		}
		const char* slash = std::strrchr(s.File, '/');
		fprintf(
			stderr,
			"  %*s%s (%s:%d)\n",
			indent * 2,
			"",
			s.Name,
			(slash != nullptr) ? slash + 1 : s.File,
			s.Line
		);
	}

	void printTree(
		const MNode& n,
		int depth,
		uint64_t frames,
		uint64_t minTotal,
		size_t& hidden
	)
	{
		for (const auto& k : n.Kids)
		{
			if (k->Total < minTotal)
			{
				++hidden;
				continue;
			}
			printRow(
				k->Total / 1e6,
				k->Self / 1e6,
				k->Calls,
				static_cast<double>(k->Total) / k->Calls / 1e3,
				k->Max / 1e3,
				frames,
				(frames != 0U) ? k->Total / 1e6 / static_cast<double>(frames)
							   : 0.0,
				depth,
				*k->Slot
			);
			printTree(*k, depth + 1, frames, minTotal, hidden);
		}
	}

	struct Reporter
	{
		~Reporter()
		{
			Profiler::Report();
		}
	};

	[[maybe_unused]] const Reporter Reporter{};

	Profiler::Scope::Scope(const Slot* s)
	{
		if (!Enabled)
		{
			return;
		}
		struct ThreadState& ts = threadState();
		Node* node = ts.Cur->Child(s);
		_node = node;
		ts.Cur = node;
		_start = nowNs();
	}

	Profiler::Scope::~Scope()
	{
		if (_node == nullptr)
		{
			return;
		}
		Node* n = static_cast<Node*>(_node);
		uint64_t dt = nowNs() - _start;
		n->Calls++;
		n->Total += dt;
		n->Self += dt - n->Pending;
		n->Max = std::max(dt, n->Max);
		n->Pending = 0;
		if (n->Parent != nullptr)
		{
			n->Parent->Pending += dt;
		}
		threadState().Cur = n->Parent;
	}

	void Profiler::Frame()
	{
		Frames.fetch_add(1, std::memory_order_relaxed);
	}

	void Profiler::Report()
	{
		if (!Enabled)
		{
			return;
		}

		MNode root;
		for (const struct ThreadState* t =
				 threads.load(std::memory_order_acquire);
			 t != nullptr;
			 t = t->Next)
		{
			mergeInto(root, t->Root);
		}
		if (root.Kids.empty())
		{
			return;
		}
		sortKids(root);

		uint64_t frames = Frames.load(std::memory_order_relaxed);

		std::unordered_map<const Profiler::Slot*, FlatRow> map;
		std::vector<const Profiler::Slot*> path;
		collectFlat(root, path, map);
		std::vector<FlatRow> rows;
		rows.reserve(map.size());
		for (auto& kv : map)
		{
			rows.push_back(kv.second);
		}
		std::sort(
			rows.begin(),
			rows.end(),
			[](const FlatRow& a, const FlatRow& b) { return a.Self > b.Self; }
		);

		const char* env = std::getenv("MC_PROFILE");
		long n = (env != nullptr) ? std::strtol(env, nullptr, 10) : 0;
		size_t top = std::min(
			rows.size(),
			n > 1 ? static_cast<size_t>(n) : static_cast<size_t>(20)
		);

		fprintf(
			stderr,
			"\n== profile: top %zu of %zu scopes by self time",
			top,
			rows.size()
		);
		if (frames != 0U)
		{
			fprintf(
				stderr,
				", %llu frames",
				static_cast<unsigned long long>(frames)
			);
		}
		fprintf(
			stderr,
			" ==\n%10s %10s %10s %9s %9s %9s  %s\n",
			"self ms",
			"total ms",
			"calls",
			"avg us",
			"max us",
			"self/frm",
			"scope"
		);
		for (size_t i = 0; i < top; ++i)
		{
			const FlatRow& r = rows[i];
			printRow(
				r.Self / 1e6,
				r.Total / 1e6,
				r.Calls,
				static_cast<double>(r.Total) / static_cast<double>(r.Outer) /
					1e3,
				r.Max / 1e3,
				frames,
				(frames != 0U) ? r.Self / 1e6 / static_cast<double>(frames)
							   : 0.0,
				0,
				*r.Slot
			);
		}

		uint64_t grand = 0;
		for (auto& k : root.Kids)
		{
			grand += k->Total;
		}
		uint64_t min_total = grand / 200;
		size_t hidden = 0;

		fprintf(
			stderr,
			"\n== mocca profile: call tree by total time ==\n%10s %10s %10s "
			"%9s %9s %9s  %s\n",
			"total ms",
			"self ms",
			"calls",
			"avg us",
			"max us",
			"total/frm",
			"scope"
		);
		printTree(root, 0, frames, min_total, hidden);

		if (hidden != 0U)
		{
			fprintf(stderr, "(%zu subtrees under 0.5%% hidden)\n", hidden);
		}
	}
}

#endif
