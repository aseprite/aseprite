// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/app_scripting.h"
#include "app/site.h"

namespace app {

namespace {

const char* kTag = "Site";

void Site_finalize(script::ContextHandle handle, void* data)
{
  auto site = (app::Site*)data;
  delete site;
}

void Site_get_sprite(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto site = (app::Site*)ctx.toUserData(0, kTag);
  if (site->sprite())
    push_sprite(ctx, site->sprite());
  else
    ctx.pushNull();
}

void Site_get_image(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto site = (app::Site*)ctx.toUserData(0, kTag);
  if (site->image())
    push_image(ctx, site->image());
  else
    ctx.pushNull();
}

const script::FunctionEntry Site_methods[] = {
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Site_props[] = {
  { "sprite", Site_get_sprite, nullptr },
  { "image", Site_get_image, nullptr },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_site_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, kTag,
                    nullptr, 0,
                    Site_methods,
                    Site_props);
}

void push_site(script::Context& ctx, app::Site& site)
{
  ctx.newObject(kTag, new app::Site(site), Site_finalize);
}

} // namespace app
