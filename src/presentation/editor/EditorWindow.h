#pragma once

#include "domain/editor/Annotation.h"
#include "presentation/icons/IIconProvider.h"

#include <functional>

#include <QAction>
#include <QCloseEvent>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

namespace snappaste {

class AnnotationCanvas;

class EditorWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(IIconProvider& iconProvider, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void setImage(const QImage& image);
    void onToolChanged(AnnotationTool tool);

signals:
    void imageEdited(const QImage& image);
    void saveRequested();
    void copyRequested();
    void pinRequested(const QImage& image);

private:
    void createToolPanel();
    void setupActions();
    void rebuildColorMenu();
    void updateColorWell(const QColor& c);
    void updateFillColorWell(const QColor& c);

    void refreshPanelUi();
    void syncPanelDefaults();

    void buildToolSection(QVBoxLayout* layout, QWidget* content, const QString& toolStyle, QVector<QToolButton*>& toolButtons);
    void buildActionSection(QVBoxLayout* layout, QWidget* content, const QString& toolStyle, QVector<QToolButton*>& actionButtons);

    // Tool panel sub-builders
    void buildColorSection(QVBoxLayout* layout, QWidget* content);
    void buildStrokeSection(QVBoxLayout* layout, QWidget* content);
    void buildArrowSection(QVBoxLayout* layout, QWidget* content);
    void buildFontSection(QVBoxLayout* layout, QWidget* content);
    void buildCropSection(QVBoxLayout* layout, QWidget* content);
    void buildZoomSection(QVBoxLayout* layout, QWidget* content);
    void buildTransformSection(QVBoxLayout* layout, QWidget* content);
    void buildLayerSection(QVBoxLayout* layout, QWidget* content);
    void buildImageAdjustSection(QVBoxLayout* layout, QWidget* content);
    void buildInfoSection(QVBoxLayout* layout, QWidget* content);
    void wireToolPanelConnections();

    QSlider* addSliderRow(QVBoxLayout* layout, QWidget* content,
                          const QString& label, int min, int max, int def,
                          std::function<void(int)> onChanged,
                          std::function<QString(int)> fmt);

    IIconProvider& iconProvider_;
    AnnotationCanvas* canvas_ = nullptr;
    QToolButton* colorBtn_ = nullptr;
    QToolButton* fillColorBtn_ = nullptr;
    QLabel* contextHint_ = nullptr;
    QLabel* imageInfoLabel_ = nullptr;

    QToolButton* undoBtn_ = nullptr;
    QToolButton* redoBtn_ = nullptr;
    QToolButton* copyBtn_ = nullptr;
    QToolButton* pinBtn_ = nullptr;
    QToolButton* saveBtn_ = nullptr;
    QToolButton* exportBtn_ = nullptr;
    QWidget* propsWidget_ = nullptr;
    QWidget* preview_ = nullptr;
    QWidget* arrowWidget_ = nullptr;
    QWidget* radiusWidget_ = nullptr;
    QWidget* fontWidget_ = nullptr;
    QWidget* cropWidget_ = nullptr;
    QAction* eyeAction_ = nullptr;
    QListWidget* layerList_ = nullptr;
    bool rebuildingLayerList_ = false;
    std::function<void(AnnotationTool)> updateToolActions_;

    QSlider* brightSlider_ = nullptr;
    QSlider* contrastSlider_ = nullptr;
    QSlider* zoomSlider_ = nullptr;
    QLabel* zoomVal_ = nullptr;

    void rebuildLayerList();
};

} // namespace snappaste
