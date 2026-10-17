#include "core/Application.h"
#include "core/Element.h"
#include "core/Hooks.h"
#include "sandbox/GLFWPlatformSurface.h"
#include <iostream>
#include "GLFW/glfw3.h"
#include "style/StyleHelpers.h"
#include "utils/Color.h"

using namespace mocca;

struct Todo
{
	int Id;
	std::string Text;
	bool Done;
};

struct TodoInputProps
{
	std::function<void(const std::string&)> OnSubmit;
};

auto TodoInput(const TodoInputProps& props) -> Element
{
	using namespace mocca::styles;
	auto [draft, setDraft] = useState(std::string{});
	auto [hovered, setHovered] = useState(false);
	auto [radii, setRadii] =
		useAnimation(hovered ? 1.0F : 0.0F, anim::spring(200.0F, 18.0F));

	return box(
		BoxDescriptor{
			.Style =
				{
					Width(percent(100)),
					Height(px(40)),
					Padding(px(4)),
					AlignItems(Alignment::FlexStart),
					JustifyContent(Justification::Center),
					BackgroundColor(
						hovered ? colors::Red : colors::Transparent
					),
					BorderRadius(px(8.0F + (radii * 16.0F))),
					Transition(
						StyleProperty::BackgroundColor,
						anim::spring(400, 18, 1.8)
					),
				},
			.Events = {
				.OnPointerEnter = [setHovered](auto& ev) -> auto
				{ setHovered(true); },
				.OnPointerLeave = [setHovered](auto& ev) -> auto
				{ setHovered(false); },
				.OnKeyDown = [setDraft, draft, props](auto& ev) -> auto
				{
					if (ev.Code == KeyCode::Backspace)
					{
						setDraft(
							[](std::string prev) -> auto
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
								[](const std::string&) -> auto
								{ return std::string{}; }
							);
						}
					}
				},
				.OnKeyRepeat = [setDraft](auto& ev) -> auto
				{
					if (ev.Code == KeyCode::Backspace)
					{
						setDraft(
							[](std::string prev) -> auto
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
						[ev](std::string prev) -> auto
						{
							prev += ev.Text;
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
							: draft,
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
	std::string label = prefix + props.Item.Text;

	return box(
		BoxDescriptor{
			.Style =
				{
					Width(percent(100)),
					Height(px(20)),
					AlignItems(Alignment::FlexStart),
					JustifyContent(Justification::Center),
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
		[setTodos, setNextId, nextId](const std::string& text) -> void
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
					Width(px(300)),
					Height(px(500)),
					Padding(px(8)),
					AlignItems(Alignment::Stretch),
					BackgroundColor(Color::Oklch(0.7, 0.1036, 171)),
				},
			.Children = {
				component(TodoInput, TodoInputProps{.OnSubmit = addTodo}),
				box({
					.Style =
						{
							Width(percent(100)),
							Height(px(400)),
							Margin(px(5)),
							AlignItems(Alignment::FlexStart),
							Overflow(OverflowType::Scroll),
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

	app.Fonts().RegisterFile(
		"../sandbox/Inter.ttf",
		{
			.Family = "Inter",
			.Weight = Range<float>{100, 900},
		}
	);

	app.Fonts().Inspect(app.Fonts().MatchStack({"Inter"}, {.Weight = 600}));

	app.On(
		ApplicationEvent::SurfaceCreated,
		[](auto* data, auto* user) -> auto
		{
			auto* surface = (Surface*)data;
			if (surface->IsExternal())
			{
				surface->SetPlatform<GLFWPlatformSurface>();
			}
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
		.Flags = SurfaceFlags::External,
		.Root = buildTodoApp,
	});

	auto* surface = app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox2",
		.Flags = SurfaceFlags::External,
		.Root = buildTodoApp,
	});

	app.RegisterSurface({
		.Width = 400,
		.Height = 300,
		.X = 100,
		.Y = 100,
		.Title = "Sandbox3",
		.Parent = surface,
		.Root = buildTodoApp,
	});

	app.Tick(0);
	app.Print();

	while (app.IsRunning())
	{
		app.Tick();
	}

	return 0;
}
