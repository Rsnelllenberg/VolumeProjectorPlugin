#pragma once

#include <QImage>
#include "actions/WidgetAction.h"
#include "actions/IntegralAction.h"
#include "actions/DecimalAction.h"
#include "actions/ToggleAction.h"
#include "InteractiveShape.h"

#include <QVBoxLayout>

using namespace mv::gui;
class TransferFunctionPlugin;


class GradientPickerAction : public WidgetAction
{
    Q_OBJECT
public:
    Q_INVOKABLE GradientPickerAction(QObject* parent, const QString& title);

    gradientData getGradientData() const;
    QImage getGradientImage() const;

    void setGradient(const gradientData& gradientData);
    void setGradientAndImage(const gradientData& gradientData, const QImage& gradientImage);

    void initialize(TransferFunctionPlugin* transferFunctionPlugin);
signals:
    void gradientChanged(const gradientData& gradientData, const QImage& gradientImage);

protected:
    void connectToPublicAction(WidgetAction* publicAction, bool recursive) override;
    void disconnectFromPublicAction(bool recursive) override;

    gradientData _gradientData;
    QImage _gradientImage;

    class Widget : public WidgetActionWidget {
    protected:
        Widget(QWidget* parent, GradientPickerAction* gradientPickerAction);

        QVBoxLayout _layout;
        ToggleAction _gradientToggleAction;
        IntegralAction _gradientTextureIDAction;
        DecimalAction _gradientXOffsetAction;
        DecimalAction _gradientYOffsetAction;
        DecimalAction _gradientWidthAction;
        DecimalAction _gradientHeightAction;
        IntegralAction _gradientRotationAction;
        QLabel* _gradientImageLabel;

        bool _updateElements;

        friend class GradientPickerAction;
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this);
    };
};

Q_DECLARE_METATYPE(GradientPickerAction)

inline const auto gradientPickerActionMetaTypeId = qRegisterMetaType<GradientPickerAction*>("GradientPickerAction*");

