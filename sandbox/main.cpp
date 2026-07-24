#include "Application.h"
#include "Element.h"
#include "StyleHelpers.h"
#include "sandbox/GLFWPlatformSurface.h"
#include <iostream>
#include "GLFW/glfw3.h"

using namespace mocca;

struct Todo
{
	int Id;
	std::u32string Text;
	bool Done;
};

struct TodoInputProps
{
	std::function<void(const std::u32string&)> OnSubmit;
};

auto TodoInput(const TodoInputProps& props) -> Element
{
	using namespace mocca::styles;
	auto [draft, setDraft] = useState(std::u32string{});
	auto [hovered, setHovered] = useState(false);

	return box(
		BoxDescriptor{
			.Style =
				{.Width = {percent(100)},
				 .Height = {px(40)},
				 .Padding = padding(px(4)),
				 .AlignItems = Alignment::FlexStart,
				 .JustifyContent = Justification::Center,
				 .BackgroundColor = hovered ? colors::Red
											: colors::Transparent},
			.Events = {
				.OnPointerDown = [setHovered](auto& ev) -> auto
				{ setHovered([](auto prev) { return !prev; }); },
				.OnKeyDown = [setDraft, draft, props](auto& ev) -> auto
				{
					if (ev.Code == KeyCode::Backspace)
					{
						setDraft(
							[](std::u32string prev) -> auto
							{
								if (!prev.empty())
								{
									prev.pop_back();
								}
								return prev;
							}
						);
					}
					else if (ev.Code == KeyCode::Enter)
					{
						if (!draft.empty())
						{
							props.OnSubmit(draft);
							setDraft(
								[](const std::u32string&) -> auto
								{ return std::u32string{}; }
							);
						}
					}
				},
				.OnKeyRepeat = [setDraft](auto& ev) -> auto
				{
					if (ev.Code == KeyCode::Backspace)
					{
						setDraft(
							[](std::u32string prev) -> auto
							{
								if (!prev.empty())
								{
									prev.pop_back();
								}
								return prev;
							}
						);
					}
				},
				.OnTextInput = [setDraft](auto& ev) -> auto
				{
					setDraft(
						[ev](std::u32string prev) -> auto
						{
							prev.push_back(ev.Codepoint);
							return prev;
						}
					);
				},
			},
			.Children = {
				text({
					.Content =
						draft.empty()
							? std::string("Type a todo and press Enter...")
							: mocca::detail::toUtf8(draft),
				}),
			},
		}
	);
}

struct TodoItemProps
{
	Todo Item;
	std::function<void(int)> OnToggle;
};

auto TodoItem(const TodoItemProps& props) -> Element
{
	using namespace mocca::styles;

	std::string prefix = props.Item.Done ? "[x] " : "[ ] ";
	std::string label = prefix + mocca::detail::toUtf8(props.Item.Text);

	return box(
		BoxDescriptor{
			.Style =
				{
					.Width = {percent(100)},
					.Height = {px(20)},
					.AlignItems = Alignment::FlexStart,
					.JustifyContent = Justification::Center,
				},
			.Events =
				{
					.OnPointerDown = [props](auto& ev) -> auto
					{ props.OnToggle(props.Item.Id); },
				},
			.Children = {
				text({.Content = label}),
			},
		}
	);
}

auto buildTodoApp() -> Element
{
	using namespace mocca::styles;

	auto [todos, setTodos] = useState(std::vector<Todo>{});
	auto [nextId, setNextId] = useState(0);

	auto addTodo =
		[setTodos, setNextId, nextId](const std::u32string& text) -> void
	{
		setTodos(
			[text, nextId](std::vector<Todo> prev) -> auto
			{
				prev.push_back({.Id = nextId, .Text = text, .Done = false});
				return prev;
			}
		);
		setNextId([](int prev) -> auto { return prev + 1; });
	};

	auto toggleTodo = [setTodos](int id) -> void
	{
		setTodos(
			[id](std::vector<Todo> prev) -> auto
			{
				for (auto& t : prev)
				{
					if (t.Id == id)
					{
						t.Done = !t.Done;
					}
				}
				return prev;
			}
		);
	};

	return box(
		BoxDescriptor{
			.Style =
				{
					.Width = {px(300)},
					.Height = {px(500)},
					.Padding = padding(px(8)),
					.AlignItems = Alignment::Stretch,
					.BackgroundColor = Color::Oklch(0.7, 0.1036, 171),
				},
			.Children = {
				component(TodoInput, TodoInputProps{.OnSubmit = addTodo}),
				box({
					.Style =
						{
							.Width = {percent(100)},
							.Height = {px(400)},
							.Margin = margin(px(5)),
							.AlignItems = Alignment::FlexStart,
							.Overflow =
								{
									OverflowType::Scroll,
								},
						},
					.Children = mapElements(
						todos,
						[toggleTodo](const Todo& item) -> Element
						{
							return component(
								TodoItem,
								TodoItemProps{
									.Item = item,
									.OnToggle = toggleTodo,
								},
								{.Key = item.Id}
							);
						}
					),
				}),
			},
		}
	);
}

void logCallback(const LogMessage& message, void* user)
{
	std::cout << message.Message << "\n";
}

auto main(int argc, const char** argv) -> int
{
	auto app = Application("com.mocca.sandbox");
	mocca::Logger::SetLogCallback(logCallback);

	app.On(
		ApplicationEvent::SurfaceCreated,
		[](auto* data, auto* user) -> auto
		{
			auto* surface = (Surface*)data;
			surface->SetPlatform<GLFWPlatformSurface>();
			return true;
		}
	);

	app.On(
		ApplicationEvent::Poll,
		[](auto* data, auto* user) -> auto
		{
			glfwPollEvents();
			return true;
		}
	);

	app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox",
		.Root = buildTodoApp,
	});

	app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox2",
		.Root = buildTodoApp,
	});

	app.Tick(0);
	app.Print();

	while (app.IsRunning())
	{
		app.Tick(0);
	}

	return 0;
}
