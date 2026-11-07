#pragma once
#include <string>
#include "fields.h"
#include "text/render.h"

namespace scn {

// the renderer's entities for every bound role (spec section 12): names, the person's noun and sex, act fields
text::Ctx text_ctx(const Ctx& x, const std::string& lang);

}
