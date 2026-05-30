#pragma once

#include <memory>

namespace snappaste {

struct InfrastructureServices;
class CaptureWorkflow;
class PinnedImageService;
class CaptureSelectionHistory;

struct DomainServices {
    std::unique_ptr<CaptureWorkflow> captureWorkflow;
    std::unique_ptr<PinnedImageService> pinnedImageService;
    std::unique_ptr<CaptureSelectionHistory> captureSelectionHistory;
};

class ServiceFactory final {
public:
    static DomainServices create(InfrastructureServices& infra);
};

} // namespace snappaste