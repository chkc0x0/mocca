#include "a11y/Snapshot.h"
#include "core/Logger.h"
#include "core/Node.h"
#include "core/Surface.h"
#include "style/ComputedStyle.h"
#include "style/StyleEnums.h"
#include "style/StyleProperty.h"
#include "yoga/YGNodeLayout.h"
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace mocca::aria
{
	struct EmitCtx
	{
		size_t AttachIndex = 0;
		Vector2 Origin{.X = 0.0F, .Y = 0.0F};
		Vector2 ScrollAccum{.X = 0.0F, .Y = 0.0F};
		bool ConsumeText = false;
	};

	auto trim(std::string_view text) -> std::string_view
	{
		auto isSpace = [](char c) -> bool
		{ return std::isspace(static_cast<unsigned char>(c)) != 0; };

		size_t begin = 0;
		size_t end = text.size();
		while (begin < end && isSpace(text[begin]))
		{
			++begin;
		}
		while (end > begin && isSpace(text[end - 1]))
		{
			--end;
		}
		return text.substr(begin, end - begin);
	}

	auto isNameFromContentsRole(Role role) -> bool
	{
		switch (role)
		{
		case Role::Button:
		case Role::CheckBox:
		case Role::RadioButton:
		case Role::Switch:
		case Role::Link:
		case Role::MenuItem:
		case Role::Tab:
		case Role::ListItem:
		case Role::TreeItem:
		case Role::ListBoxOption:
			return true;
		default:
			return false;
		}
	}

	auto isDisplayNone(const detail::Node& node) -> bool
	{
		return node.Style.Get<styles::StyleProperty::Display>() ==
			   styles::DisplayType::None;
	}

	auto isA11yHidden(const detail::Node& node) -> bool
	{
		return node.A11y != nullptr && node.A11y->Hidden;
	}

	auto isPruned(const detail::Node& node) -> bool
	{
		return isDisplayNone(node) || isA11yHidden(node);
	}

	auto scrolls(const detail::Node& node) -> bool
	{
		return node.IsBox() &&
			   node.Style.Get<styles::StyleProperty::Overflow>() ==
				   styles::OverflowType::Scroll;
	}

	auto collectName(const detail::Node& node, std::vector<std::string>& out)
		-> void
	{
		for (const auto& child : node.Children)
		{
			if (child == nullptr)
			{
				continue;
			}

			if (child->IsComponent())
			{
				collectName(*child, out);
				continue;
			}

			if (child->A11y != nullptr)
			{
				continue;
			}

			if (child->IsText())
			{
				auto text = trim(std::get<TextElement>(child->Kind).Content);
				if (!text.empty())
				{
					out.emplace_back(text);
				}
				continue;
			}

			if (isPruned(*child))
			{
				continue;
			}

			collectName(*child, out);
		}
	}

	auto joinName(const std::vector<std::string>& parts) -> std::string
	{
		std::string name;
		for (const auto& part : parts)
		{
			if (part.empty())
			{
				continue;
			}
			if (!name.empty())
			{
				name.push_back(' ');
			}
			name += part;
		}
		return name;
	}

	class SnapshotBuilder
	{
	public:
		auto Build(const Surface& surface) -> std::shared_ptr<const Snapshot>;

	private:
		static auto Bounds(const detail::Node& node, const EmitCtx& ctx) 
			-> Rectangle;
		auto _emitNode(Node node, const EmitCtx& ctx) -> size_t;
		void _visit(const detail::Node& node, EmitCtx ctx);
		void _flattenSurface(const Surface& surface, EmitCtx ctx);
		void _resolveRelations();
		auto _resolveFocus(const Surface& surface) -> detail::NodeId;

		Snapshot _snapshot;
		std::unordered_map<detail::NodeId, detail::NodeId> _focusMap;
	};

	auto SnapshotBuilder::Bounds(
		const detail::Node& node,
		const EmitCtx& ctx
	)  -> Rectangle
	{
		float width = 0.0F;
		float height = 0.0F;
		if (node.YogaNode != nullptr)
		{
			width = YGNodeLayoutGetWidth(node.YogaNode);
			height = YGNodeLayoutGetHeight(node.YogaNode);
		}

		return {
			.X = node.LayoutPos.X - ctx.ScrollAccum.X + ctx.Origin.X,
			.Y = node.LayoutPos.Y - ctx.ScrollAccum.Y + ctx.Origin.Y,
			.Width = width,
			.Height = height,
		};
	}

	auto SnapshotBuilder::_emitNode(Node node, const EmitCtx& ctx) -> size_t
	{
		detail::NodeId id = node.Id;
		size_t index = _snapshot.Nodes.size();
		_snapshot.Nodes.push_back(std::move(node));
		_snapshot.Index[id] = index;
		_snapshot.Nodes[ctx.AttachIndex].Children.push_back(id);
		return index;
	}

	void SnapshotBuilder::_visit(const detail::Node& node, EmitCtx ctx)
	{
		_focusMap[node.Id] = _snapshot.Nodes[ctx.AttachIndex].Id;

		if (node.IsComponent())
		{
			for (const auto& child : node.Children)
			{
				if (child != nullptr)
				{
					_visit(*child, ctx);
				}
			}
			return;
		}

		if (node.IsText())
		{
			if ((ctx.ConsumeText && node.A11y == nullptr) || isPruned(node))
			{
				return;
			}

			Node emitted;
			emitted.Id = node.Id;
			emitted.Role = node.A11y != nullptr && node.A11y->Role != Role::None
							   ? node.A11y->Role
							   : Role::Label;
			emitted.TextValue = std::get<TextElement>(node.Kind).Content;
			emitted.Bounds = Bounds(node, ctx);
			if (node.A11y != nullptr)
			{
				emitted.Props = node.A11y;
				emitted.Name = node.A11y->Name;
			}

			_emitNode(std::move(emitted), ctx);
			_focusMap[node.Id] = node.Id;
			return;
		}

		if (isPruned(node))
		{
			return;
		}

		bool emit = false;
		Role role = Role::None;
		std::string name;
		bool inferName = false;

		if (node.A11y != nullptr)
		{
			if (node.A11y->Role != Role::None)
			{
				role = node.A11y->Role;
				name = node.A11y->Name;
				inferName = name.empty() && isNameFromContentsRole(role);
				emit = true;
			}
			else if (!node.A11y->Name.empty())
			{
				role = Role::Group;
				name = node.A11y->Name;
				emit = true;
			}
		}

		EmitCtx childCtx = ctx;
		if (scrolls(node))
		{
			childCtx.ScrollAccum.X += node.ScrollOffset.X;
			childCtx.ScrollAccum.Y += node.ScrollOffset.Y;
		}

		if (emit)
		{
			Node emitted;
			emitted.Id = node.Id;
			emitted.Role = role;
			emitted.Name = std::move(name);
			emitted.Bounds = Bounds(node, ctx);
			emitted.Props = node.A11y;

			if (inferName)
			{
				std::vector<std::string> parts;
				collectName(node, parts);
				emitted.Name = joinName(parts);
			}

			childCtx.ConsumeText = isNameFromContentsRole(role);

			childCtx.AttachIndex = _emitNode(std::move(emitted), ctx);
			_focusMap[node.Id] = node.Id;
		}

		for (const auto& child : node.Children)
		{
			if (child != nullptr)
			{
				_visit(*child, childCtx);
			}
		}
	}

	void SnapshotBuilder::_flattenSurface(const Surface& surface, EmitCtx ctx)
	{
		if (const auto* root = surface.Tree(); root != nullptr)
		{
			_visit(*root, ctx);
		}

		for (const auto& child : surface._children)
		{
			if (child == nullptr || child->IsPlatformBacked() ||
				child->GetState() != SurfaceState::Alive)
			{
				continue;
			}

			auto desc = child->GetDescriptor();
			EmitCtx childCtx = ctx;
			childCtx.Origin.X += desc.X;
			childCtx.Origin.Y += desc.Y;
			childCtx.ScrollAccum = {.X = 0.0F, .Y = 0.0F};
			childCtx.ConsumeText = false;
			_flattenSurface(*child, childCtx);
		}
	}

	auto SnapshotBuilder::_resolveFocus(const Surface& surface) -> detail::NodeId
	{
		detail::NodeId focused = surface.GetEffectiveFocus();
		auto it = _focusMap.find(focused);
		return it != _focusMap.end() ? it->second : 0;
	}

	void SnapshotBuilder::_resolveRelations()
	{
		std::unordered_map<uint64_t, detail::NodeId> byPropsId;
		std::unordered_set<uint64_t> warned;

		for (const auto& node : _snapshot.Nodes)
		{
			if (node.Props == nullptr || node.Props->Id == 0)
			{
				continue;
			}

			auto [it, inserted] = byPropsId.emplace(node.Props->Id, node.Id);
			if (!inserted && warned.insert(node.Props->Id).second)
			{
				mc_warning(
					"duplicate aria id {} (nodes #{} and #{}); using the first one",
					node.Props->Id,
					it->second,
					node.Id
				);
			}
		}

		auto resolve = [&byPropsId](
						   const std::vector<uint64_t>& ids
					   ) -> std::vector<detail::NodeId>
		{
			std::vector<detail::NodeId> out;
			out.reserve(ids.size());
			for (uint64_t id : ids)
			{
				auto it = byPropsId.find(id);
				if (it != byPropsId.end())
				{
					out.push_back(it->second);
				}
			}
			return out;
		};

		for (auto& node : _snapshot.Nodes)
		{
			if (node.Props == nullptr)
			{
				continue;
			}

			node.LabelledBy = resolve(node.Props->LabelledBy);
			node.DescribedBy = resolve(node.Props->DescribedBy);
			node.Controls = resolve(node.Props->Controls);
			node.Details = resolve(node.Props->Details);
			node.Owns = resolve(node.Props->Owns);
			node.FlowTo = resolve(node.Props->FlowTo);
		}
	}

	auto SnapshotBuilder::Build(const Surface& surface)
		-> std::shared_ptr<const Snapshot>
	{
		auto desc = surface.GetDescriptor();

		_snapshot = Snapshot{};
		_snapshot.Root = 0;
		_snapshot.Title = desc.Title;

		Node window;
		window.Id = 0;
		window.Role = Role::Window;
		window.Name = desc.Title;
		window.Bounds = {
			.X = 0.0F,
			.Y = 0.0F,
			.Width = desc.Width,
			.Height = desc.Height,
		};
		_snapshot.Nodes.push_back(std::move(window));
		_snapshot.Index[0] = 0;

		EmitCtx ctx{};
		ctx.AttachIndex = 0;
		_flattenSurface(surface, ctx);

		_snapshot.Focus = _resolveFocus(surface);
		_resolveRelations();

		return std::make_shared<const Snapshot>(std::move(_snapshot));
	}

	auto Snapshot::Find(detail::NodeId id) const -> const Node*
	{
		auto it = Index.find(id);
		return it != Index.end() ? &Nodes[it->second] : nullptr;
	}

	auto Snapshot::Build(const Surface& surface)
		-> std::shared_ptr<const Snapshot>
	{
		SnapshotBuilder builder;
		return builder.Build(surface);
	}
}
