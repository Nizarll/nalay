#pragma once

inline std::ostream& operator<<(std::ostream& os, const base_style& s) {
    return os << "color("
              << s.color.x << "," << s.color.y << ","
              << s.color.z << "," << s.color.w << ")";
}

// primitive variants
inline std::ostream& operator<<(std::ostream& os, const primitive::text& t) {
    return os << "text(\"" << t.text << "\")";
}

inline std::ostream& operator<<(std::ostream& os, const primitive::img& i) {
    return os << "img(format=" << i.format << ")";
}

inline std::ostream& operator<<(std::ostream& os, const primitive::rect& r) {
    return os << "rect(" << r.width << "x" << r.height << ")";
}

// render_cmd
inline std::ostream& operator<<(std::ostream& os, const render_cmd& cmd) {
    os << "{ content=";
    std::visit([&](auto&& arg){ os << arg; }, cmd.content);
    os << ", style=" << cmd.style
       << ", size=" << cmd.size
       << ", pos=" << cmd.pos
       << " }";
    return os;
}
