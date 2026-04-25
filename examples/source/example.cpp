#include <format>
#include <filesystem>
#include <iostream>
#include <string>
#include <raylib.h>
#include "nalay/nalay.hpp"
#include "nalay/backends/raylib.hpp"
namespace fs = std::filesystem;

namespace everforest {
static constexpr nalay::color bg0_hard   = nalay::color::from_hex(0x272E33FF);
static constexpr nalay::color bg0        = nalay::color::from_hex(0x2F383EFF);
static constexpr nalay::color bg1        = nalay::color::from_hex(0x374145FF);
static constexpr nalay::color bg2        = nalay::color::from_hex(0x414B50FF);
static constexpr nalay::color bg3        = nalay::color::from_hex(0x495156FF);
static constexpr nalay::color fg         = nalay::color::from_hex(0xD3C6AAFF);
static constexpr nalay::color red        = nalay::color::from_hex(0xE67E80FF);
static constexpr nalay::color orange     = nalay::color::from_hex(0xE69875FF);
static constexpr nalay::color yellow     = nalay::color::from_hex(0xDBBC7FFF);
static constexpr nalay::color green      = nalay::color::from_hex(0xA7C080FF);
static constexpr nalay::color aqua       = nalay::color::from_hex(0x83C092FF);
static constexpr nalay::color blue       = nalay::color::from_hex(0x7FBBB3FF);
static constexpr nalay::color purple     = nalay::color::from_hex(0xD699B6FF);
}

using namespace nalay;
using namespace nalay::ui;

auto create_header()
{
  return rows(
    label("Nalay UI Framework Demo")
    .color(everforest::fg)
    .font_size(32)
    .padding(padding::all(20))
    .bg_color(everforest::bg0_hard),

    label("Comprehensive showcase of UI components and features")
    .color(everforest::aqua)
    .font_size(14)
    .padding(padding(10, 20, 4, 20))
    .bg_color(everforest::bg0_hard)
  ).bg_color(everforest::bg0_hard);
}

auto create_image_section(raylib_image_provider& images)
{
  return columns(
    rows(
      label("Image Display").color(everforest::green).font_size(18).padding(padding::all(8)),

      columns(
        image(images.get("gradient1"))
        .size(unit::px(120), unit::px(120))
        .padding(padding::all(10))
        .bg_color(everforest::bg1)
        .border_radius(10),

        image(images.get("gradient2"))
        .size(unit::px(120), unit::px(120))
        .padding(padding::all(10))
        .bg_color(everforest::bg1)
        .border_radius(10)
      ).padding(padding::all(8))
    ).bg_color(everforest::bg2)
    .padding(padding::all(12))
    .border_radius(8)
  );
}

auto create_buttons_section(nalay::reactive<int>& click_count)
{
  return rows(
    label("Interactive Buttons").color(everforest::green).font_size(18).padding(padding::all(8)),
    columns(
      button("Primary")
      .bg_color(everforest::blue)
      .color(everforest::fg)
      .padding(padding(12, 24))
      .border_radius(6)
      .clicked([&]() { click_count.set(click_count.get() + 1); std::cout << "Primary clicked! Count: " << click_count.get() << "\n"; }),

      button("Success")
      .bg_color(everforest::green)
      .color(everforest::bg0_hard)
      .padding(padding(12, 24))
      .border_radius(6)
      .clicked([]() { std::cout << "Success!\n"; }),

      button("Warning")
      .bg_color(everforest::yellow)
      .color(everforest::bg0_hard)
      .padding(padding(12, 24))
      .border_radius(6)
      .clicked([]() { std::cout << "Warning!\n"; }),

      button("Danger")
      .bg_color(everforest::red)
      .color(everforest::fg)
      .padding(padding(12, 24))
      .border_radius(6)
      .clicked([]() { std::cout << "Danger!\n"; })
    ).padding(padding::all(8))
  ).bg_color(everforest::bg2)
  .padding(padding::all(12))
  .border_radius(8);
}

auto create_inputs_section()
{
  return rows(
    label("Input Fields").color(everforest::green).font_size(18).padding(padding::all(8)),

    columns(
      rows(
        label("Name:").color(everforest::fg).padding(padding(4, 8)),
        input<std::string>()
        .size(unit::px(200), unit::px(32))
        .bg_color(everforest::bg1)
        .color(everforest::fg)
        .padding(padding(8, 12))
        .border_radius(6)
        .value_changed([](const std::string& val) {
          std::cout << "Name changed: " << val << "\n";
        })
      ).padding(padding::all(4)),

      rows(
        label("Age:").color(everforest::fg).padding(padding(4, 8)),
        input<int>()
        .size(unit::px(100), unit::px(32))
        .bg_color(everforest::bg1)
        .color(everforest::fg)
        .padding(padding(8, 12))
        .border_radius(6)
        .value_changed([](int val) {
          std::cout << "Age changed: " << val << "\n";
        })
      ).padding(padding::all(4))
    ).padding(padding::all(8))
  ).bg_color(everforest::bg2)
  .padding(padding::all(12))
  .border_radius(8);
}

auto create_color_cards()
{
  return columns(
    rows(
      columns()
      .size(unit::px(100), unit::px(80))
      .bg_color(everforest::red)
      .border_radius(8)
      .padding(padding::all(4)),
      label("Red").color(everforest::fg).padding(padding::all(4))
    ).padding(padding::all(6)),

    rows(
      columns()
      .size(unit::px(100), unit::px(80))
      .bg_color(everforest::green)
      .border_radius(8)
      .padding(padding::all(4)),
      label("Green").color(everforest::fg).padding(padding::all(4))
    ).padding(padding::all(6)),

    rows(
      columns()
      .size(unit::px(100), unit::px(80))
      .bg_color(everforest::blue)
      .border_radius(8)
      .padding(padding::all(4)),
      label("Blue").color(everforest::fg).padding(padding::all(4))
    ).padding(padding::all(6)),

    rows(
      columns()
      .size(unit::px(100), unit::px(80))
      .bg_color(everforest::purple)
      .border_radius(8)
      .padding(padding::all(4)),
      label("Purple").color(everforest::fg).padding(padding::all(4))
    ).padding(padding::all(6))
  ).bg_color(everforest::bg2)
  .padding(padding::all(12))
  .border_radius(8);
}

auto create_counter_display(nalay::reactive<int>& click_count)
{
  return rows(
    label(nalay::reactive<std::string>([&] {
      return std::format("Click count: {}", click_count.get());
    }, click_count))
    .color(everforest::aqua)
    .font_size(16)
    .padding(padding::all(12))
  ).bg_color(everforest::bg2)
  .border_radius(8)
  .padding(padding(8, 8, 8, 8));
}

void setup_demo_images(raylib_image_provider& images)
{
  Image demo_img1 = GenImageGradientRadial(128, 128, 0.0f, SKYBLUE, BLUE);
  auto demo_path1 = "demo_gradient1.png";
  ExportImage(demo_img1, demo_path1);
  UnloadImage(demo_img1);

  Image demo_img2 = GenImageGradientLinear(128, 128, 0, PURPLE, VIOLET);
  auto demo_path2 = "demo_gradient2.png";
  ExportImage(demo_img2, demo_path2);
  UnloadImage(demo_img2);

  images.load("gradient1", demo_path1);
  images.load("gradient2", demo_path2);
}

auto main() -> int
{
  raylib_renderer renderer{};
  nalay::render_queue queue;
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(1400, 900, "Nalay UI Demo - Comprehensive Example");
  SetTargetFPS(60);

  raylib_font_provider fonts{};
  raylib_input_provider inputs{};
  raylib_image_provider images{};

  nalay::context::instance().init(fonts, inputs, images);

  fonts.load("roboto", "examples/resources/Roboto.ttf");
  fonts.set_default("examples/resources/Roboto.ttf");

  setup_demo_images(images);

  nalay::reactive<int> click_count = nalay::reactive(0);

  auto header = create_header();
  auto image_section = create_image_section(images);
  auto buttons_section = create_buttons_section(click_count);
  auto inputs_section = create_inputs_section();
  auto color_cards = create_color_cards();
  auto counter_display = create_counter_display(click_count);

  auto layout = rows(
    std::move(header),
    columns(
      rows(
        std::move(image_section),
        std::move(buttons_section)
      ).padding(padding::all(8)),
      rows(
        std::move(inputs_section),
        std::move(color_cards),
        std::move(counter_display)
      ).padding(padding::all(8))
    ).padding(padding::all(12))
  ).bg_color(everforest::bg3);

  while (not WindowShouldClose()) {
    float delta = GetFrameTime();
    nalay::context::instance().delta_time = delta;
    queue.reset();
    inputs.poll();

    layout.size(unit::px(GetScreenWidth()), unit::px(GetScreenHeight()));
    layout.compute(queue);
    nalay::poll(layout);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    renderer.render(queue);
    DrawText(std::format("FPS: {}", GetFPS()).c_str(), 10, GetScreenHeight() - 30, 20, GREEN);
    EndDrawing();
  }

  return 0;
}
