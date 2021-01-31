#pragma once

namespace rostrum {
  // the only internal rostrum module. the core module
  [[nodiscard]]
  sol::table imbue_core(sol::state_view& lua);
}
