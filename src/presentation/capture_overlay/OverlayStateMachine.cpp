#include "presentation/capture_overlay/OverlayStateMachine.h"

namespace snappaste {

void OverlayStateMachine::pushSelectionUndo()
{
    selectionUndoStack_.push_back(dragStartSelection_);
    if (selectionUndoStack_.size() > kMaxSelectionUndo) {
        selectionUndoStack_.removeFirst();
    }
}

bool OverlayStateMachine::undoSelection()
{
    if (selectionUndoStack_.isEmpty()) {
        return false;
    }
    dragStartSelection_ = selectionUndoStack_.takeLast();
    return true;
}

} // namespace snappaste