#pragma once

#include <QPoint>
#include <QRect>
#include <QVector>

namespace snappaste {

class OverlayStateMachine final {
public:
    enum class State {
        Idle,
        CandidatePressed,
        Selecting,
        Moving,
        Resizing,
        Ready,
        ActionPending,
        Cancelled
    };

    enum class Handle {
        None,
        Inside,
        TopLeft, Top, TopRight,
        Right, BottomRight, Bottom,
        BottomLeft, Left
    };

    State currentState() const noexcept { return state_; }
    void setState(State s) noexcept { state_ = s; }
    bool isIdle() const noexcept { return state_ == State::Idle; }
    bool isReady() const noexcept { return state_ == State::Ready; }
    bool isSelecting() const noexcept { return state_ == State::Selecting; }
    bool isActive() const noexcept {
        return state_ != State::Idle && state_ != State::Cancelled;
    }

    QPoint pressGlobal() const noexcept { return pressGlobal_; }
    void setPressGlobal(QPoint p) noexcept { pressGlobal_ = p; }

    QRect pressedCandidate() const noexcept { return pressedCandidate_; }
    void setPressedCandidate(QRect r) noexcept { pressedCandidate_ = r; }

    QPoint dragStart() const noexcept { return dragStart_; }
    void setDragStart(QPoint p) noexcept { dragStart_ = p; }

    QRect dragStartSelection() const noexcept { return dragStartSelection_; }
    void setDragStartSelection(QRect r) noexcept { dragStartSelection_ = r; }

    Handle activeHandle() const noexcept { return activeHandle_; }
    void setActiveHandle(Handle h) noexcept { activeHandle_ = h; }

    int smartCandidateIndex() const noexcept { return smartCandidateIndex_; }
    void setSmartCandidateIndex(int i) noexcept { smartCandidateIndex_ = i; }
    void clearCandidates() noexcept {
        smartCandidates_.clear();
        smartCandidateIndex_ = -1;
    }
    const QVector<QRect>& smartCandidates() const noexcept { return smartCandidates_; }
    void setSmartCandidates(QVector<QRect> c) noexcept { smartCandidates_ = std::move(c); }

    void pushSelectionUndo();
    bool undoSelection();

private:
    State state_ = State::Idle;
    Handle activeHandle_ = Handle::None;
    QPoint pressGlobal_;
    QRect pressedCandidate_;
    QPoint dragStart_;
    QRect dragStartSelection_;
    QVector<QRect> smartCandidates_;
    int smartCandidateIndex_ = -1;
    QVector<QRect> selectionUndoStack_;
    static constexpr int kMaxSelectionUndo = 20;
};

} // namespace snappaste