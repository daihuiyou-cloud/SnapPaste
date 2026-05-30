#pragma once

#include "presentation/icons/IIconProvider.h"

namespace snappaste {

class IconProvider final : public IIconProvider {
public:
    QIcon icon(IconName name) override;
};

} // namespace snappaste
