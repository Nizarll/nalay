#include <format>
#include <filesystem>
#include <iostream>
#include <string>
#include <raylib.h>
#include "nalay/nalay.hpp"
#include "nalay/backends/raylib.hpp"

namespace fs = std::filesystem;

namespace nf {
  // Everforest Dark Hard palette
  static constexpr nalay::color bg0     = nalay::color::from_hex(0x272E33FF);
  static constexpr nalay::color bg1     = nalay::color::from_hex(0x2E383CFF);
  static constexpr nalay::color bg2     = nalay::color::from_hex(0x374145FF);
  static constexpr nalay::color fg      = nalay::color::from_hex(0xD3C6AAFF);
  static constexpr nalay::color red     = nalay::color::from_hex(0xE67E80FF);
  static constexpr nalay::color green   = nalay::color::from_hex(0xA7C080FF);
  static constexpr nalay::color blue    = nalay::color::from_hex(0x7FBBB3FF);
  static constexpr nalay::color orange  = nalay::color::from_hex(0xE69875FF);
}


namespace everforest {

static constexpr nalay::color bg0_hard   = nalay::color::from_hex(0x272E33FF);
static constexpr nalay::color bg0        = nalay::color::from_hex(0x2F383EFF);
static constexpr nalay::color bg0_soft   = nalay::color::from_hex(0x323C41FF);
static constexpr nalay::color bg1        = nalay::color::from_hex(0x374145FF);
static constexpr nalay::color bg2        = nalay::color::from_hex(0x414B50FF);
static constexpr nalay::color bg3        = nalay::color::from_hex(0x495156FF);
static constexpr nalay::color bg4        = nalay::color::from_hex(0x4F5B58FF);

static constexpr nalay::color fg         = nalay::color::from_hex(0xD3C6AAFF);
static constexpr nalay::color fg_dim     = nalay::color::from_hex(0x9DA9A0FF);
static constexpr nalay::color fg_dark    = nalay::color::from_hex(0x7A8478FF);

static constexpr nalay::color red        = nalay::color::from_hex(0xE67E80FF);
static constexpr nalay::color orange     = nalay::color::from_hex(0xE69875FF);
static constexpr nalay::color yellow     = nalay::color::from_hex(0xDBBC7FFF);
static constexpr nalay::color green      = nalay::color::from_hex(0xA7C080FF);
static constexpr nalay::color aqua       = nalay::color::from_hex(0x83C092FF);
static constexpr nalay::color blue       = nalay::color::from_hex(0x7FBBB3FF);
static constexpr nalay::color purple     = nalay::color::from_hex(0xD699B6FF);

static constexpr nalay::color grey0      = nalay::color::from_hex(0x7A8478FF);
static constexpr nalay::color grey1      = nalay::color::from_hex(0x859289FF);
static constexpr nalay::color grey2      = nalay::color::from_hex(0x9DA9A0FF);

static constexpr nalay::color border     = nalay::color::from_hex(0x3C4841FF);
static constexpr nalay::color cursor     = nalay::color::from_hex(0xD3C6AAFF);
static constexpr nalay::color selection  = nalay::color::from_hex(0x4C565CFF);

static constexpr nalay::color error      = red;
static constexpr nalay::color warning    = yellow;
static constexpr nalay::color success    = green;
static constexpr nalay::color info       = blue;

}

auto main() -> int
{
  raylib_renderer renderer{};
  nalay::render_queue queue;
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(1200, 900, "alignment demo");
  SetTargetFPS(100);

  raylib_font_provider fonts{};
  raylib_input_provider inputs{};
  nalay::context ctx{ .fonts = fonts, .inputs = inputs, .node_allocator = {} };
  fonts.load("roboto", "test/Roboto-Black.ttf");
  auto font = std::reference_wrapper(fonts.get("roboto"));
  fonts.set_default("test/Roboto-Black.ttf");

  nalay::set_ctx(&ctx);

  using namespace nalay;
  using namespace nalay::ui;

  auto containers = columns().padding(padding::all(10));

  for (int i = 0; i < 4; i++)
    containers.add(
      columns(
      ).size(100, 200)
      .bg_color(everforest::bg1)
      .border_radius(15)
    );

  auto layout = rows(
    rows(
      columns(
        input<std::string>().size(200, 24)
        .display_font(font)
        .y_align(alignment::center)
        .color(everforest::bg0_hard)
        .bg_color(everforest::bg1)
        .padding(padding(6, 12))
        .size(150, 24)
        .border_radius(15)
      ).padding(padding::all(8)),
      std::move(containers)
    )
  ).bg_color(everforest::bg2);


  while (not WindowShouldClose()) {
    float delta = GetFrameTime();
    ctx.delta_time = delta;
    queue.reset();
    inputs.poll();
    layout.size(GetScreenWidth(), GetScreenHeight());
    layout.measure();
    layout.place({0, 0}); //TODO: replace this garbage
    nalay::poll(layout);
    layout.emit(queue); // TODO: replace with nalay::emit(queue)
    BeginDrawing();
    ClearBackground(BLACK);
    renderer.render(queue);
    DrawText(std::format("FPS: {}", GetFPS()).c_str(), 10, 860, 20, RED);
    EndDrawing();
  }

  return 0;
}
