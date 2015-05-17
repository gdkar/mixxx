/********************************************************************************
** Form generated from reading UI file 'dlgcontrollerlearning.ui'
**
** Created by: Qt User Interface Compiler version 5.4.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DLGCONTROLLERLEARNING_H
#define UI_DLGCONTROLLERLEARNING_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DlgControllerLearning
{
public:
    QGridLayout *gridLayout;
    QStackedWidget *stackedWidget;
    QWidget *page1Choose;
    QGridLayout *gridLayout_5;
    QSpacerItem *verticalSpacer_2;
    QPushButton *pushButtonStartLearn;
    QPushButton *pushButtonClose;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_2;
    QComboBox *comboBoxChosenControl;
    QPushButton *pushButtonChooseControl;
    QLabel *labelMappingHelp;
    QLabel *labelErrorText;
    QLabel *labelDescription;
    QWidget *page2Learn;
    QVBoxLayout *verticalLayout_2;
    QLabel *control_controlToMapMessage;
    QLabel *label;
    QProgressBar *progressBarWiggleFeedback;
    QPushButton *pushButtonFakeControl2;
    QPushButton *pushButtonFakeControl;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout3;
    QPushButton *pushButtonCancelLearn;
    QSpacerItem *horizontalSpacer_3;
    QWidget *page3Confirm;
    QVBoxLayout *verticalLayout;
    QLabel *labelMappedTo;
    QLabel *labelNextHelp;
    QGroupBox *midiOptions;
    QGridLayout *gridLayout_2;
    QCheckBox *midiOptionSwitchMode;
    QCheckBox *midiOptionSoftTakeover;
    QCheckBox *midiOptionInvert;
    QCheckBox *midiOptionSelectKnob;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout;
    QPushButton *pushButtonRetry;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *pushButtonLearnAnother;
    QPushButton *pushButtonClose_2;

    void setupUi(QDialog *DlgControllerLearning)
    {
        if (DlgControllerLearning->objectName().isEmpty())
            DlgControllerLearning->setObjectName(QStringLiteral("DlgControllerLearning"));
        DlgControllerLearning->resize(542, 369);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DlgControllerLearning->sizePolicy().hasHeightForWidth());
        DlgControllerLearning->setSizePolicy(sizePolicy);
        DlgControllerLearning->setMinimumSize(QSize(0, 300));
        DlgControllerLearning->setMaximumSize(QSize(16777215, 16777215));
        gridLayout = new QGridLayout(DlgControllerLearning);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        stackedWidget = new QStackedWidget(DlgControllerLearning);
        stackedWidget->setObjectName(QStringLiteral("stackedWidget"));
        page1Choose = new QWidget();
        page1Choose->setObjectName(QStringLiteral("page1Choose"));
        sizePolicy.setHeightForWidth(page1Choose->sizePolicy().hasHeightForWidth());
        page1Choose->setSizePolicy(sizePolicy);
        gridLayout_5 = new QGridLayout(page1Choose);
        gridLayout_5->setObjectName(QStringLiteral("gridLayout_5"));
        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_5->addItem(verticalSpacer_2, 6, 0, 1, 3);

        pushButtonStartLearn = new QPushButton(page1Choose);
        pushButtonStartLearn->setObjectName(QStringLiteral("pushButtonStartLearn"));
        pushButtonStartLearn->setMinimumSize(QSize(90, 0));
        pushButtonStartLearn->setDefault(true);

        gridLayout_5->addWidget(pushButtonStartLearn, 12, 2, 1, 1);

        pushButtonClose = new QPushButton(page1Choose);
        pushButtonClose->setObjectName(QStringLiteral("pushButtonClose"));

        gridLayout_5->addWidget(pushButtonClose, 12, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_5->addItem(horizontalSpacer, 12, 1, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        comboBoxChosenControl = new QComboBox(page1Choose);
        comboBoxChosenControl->setObjectName(QStringLiteral("comboBoxChosenControl"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(comboBoxChosenControl->sizePolicy().hasHeightForWidth());
        comboBoxChosenControl->setSizePolicy(sizePolicy1);
        comboBoxChosenControl->setEditable(true);
        comboBoxChosenControl->setInsertPolicy(QComboBox::NoInsert);

        horizontalLayout_2->addWidget(comboBoxChosenControl);

        pushButtonChooseControl = new QPushButton(page1Choose);
        pushButtonChooseControl->setObjectName(QStringLiteral("pushButtonChooseControl"));
        pushButtonChooseControl->setMouseTracking(false);

        horizontalLayout_2->addWidget(pushButtonChooseControl);

        horizontalLayout_2->setStretch(0, 3);

        gridLayout_5->addLayout(horizontalLayout_2, 3, 0, 1, 3);

        labelMappingHelp = new QLabel(page1Choose);
        labelMappingHelp->setObjectName(QStringLiteral("labelMappingHelp"));
        sizePolicy.setHeightForWidth(labelMappingHelp->sizePolicy().hasHeightForWidth());
        labelMappingHelp->setSizePolicy(sizePolicy);
        labelMappingHelp->setText(QStringLiteral("(html help)"));
        labelMappingHelp->setWordWrap(true);

        gridLayout_5->addWidget(labelMappingHelp, 0, 0, 2, 3);

        labelErrorText = new QLabel(page1Choose);
        labelErrorText->setObjectName(QStringLiteral("labelErrorText"));
        labelErrorText->setText(QStringLiteral("(error text)"));

        gridLayout_5->addWidget(labelErrorText, 5, 0, 1, 3);

        labelDescription = new QLabel(page1Choose);
        labelDescription->setObjectName(QStringLiteral("labelDescription"));
        labelDescription->setText(QStringLiteral("(description text)"));

        gridLayout_5->addWidget(labelDescription, 4, 0, 1, 3);

        stackedWidget->addWidget(page1Choose);
        page2Learn = new QWidget();
        page2Learn->setObjectName(QStringLiteral("page2Learn"));
        sizePolicy.setHeightForWidth(page2Learn->sizePolicy().hasHeightForWidth());
        page2Learn->setSizePolicy(sizePolicy);
        verticalLayout_2 = new QVBoxLayout(page2Learn);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        control_controlToMapMessage = new QLabel(page2Learn);
        control_controlToMapMessage->setObjectName(QStringLiteral("control_controlToMapMessage"));
        sizePolicy.setHeightForWidth(control_controlToMapMessage->sizePolicy().hasHeightForWidth());
        control_controlToMapMessage->setSizePolicy(sizePolicy);
#ifndef QT_NO_TOOLTIP
        control_controlToMapMessage->setToolTip(QStringLiteral(""));
#endif // QT_NO_TOOLTIP
        control_controlToMapMessage->setText(QStringLiteral("(control ready to map message goes here)"));
        control_controlToMapMessage->setWordWrap(true);

        verticalLayout_2->addWidget(control_controlToMapMessage);

        label = new QLabel(page2Learn);
        label->setObjectName(QStringLiteral("label"));
        label->setWordWrap(true);

        verticalLayout_2->addWidget(label);

        progressBarWiggleFeedback = new QProgressBar(page2Learn);
        progressBarWiggleFeedback->setObjectName(QStringLiteral("progressBarWiggleFeedback"));
        progressBarWiggleFeedback->setMaximum(10);
        progressBarWiggleFeedback->setValue(0);

        verticalLayout_2->addWidget(progressBarWiggleFeedback);

        pushButtonFakeControl2 = new QPushButton(page2Learn);
        pushButtonFakeControl2->setObjectName(QStringLiteral("pushButtonFakeControl2"));
        pushButtonFakeControl2->setText(QStringLiteral("Debug Fake Pushbutton Control2"));

        verticalLayout_2->addWidget(pushButtonFakeControl2);

        pushButtonFakeControl = new QPushButton(page2Learn);
        pushButtonFakeControl->setObjectName(QStringLiteral("pushButtonFakeControl"));
        pushButtonFakeControl->setText(QStringLiteral("Debug Fake Pushbutton Control"));

        verticalLayout_2->addWidget(pushButtonFakeControl);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        horizontalLayout3 = new QHBoxLayout();
        horizontalLayout3->setSpacing(10);
        horizontalLayout3->setObjectName(QStringLiteral("horizontalLayout3"));
        pushButtonCancelLearn = new QPushButton(page2Learn);
        pushButtonCancelLearn->setObjectName(QStringLiteral("pushButtonCancelLearn"));

        horizontalLayout3->addWidget(pushButtonCancelLearn);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout3->addItem(horizontalSpacer_3);


        verticalLayout_2->addLayout(horizontalLayout3);

        verticalLayout_2->setStretch(2, 2);
        verticalLayout_2->setStretch(3, 3);
        verticalLayout_2->setStretch(4, 4);
        verticalLayout_2->setStretch(5, 5);
        stackedWidget->addWidget(page2Learn);
        page3Confirm = new QWidget();
        page3Confirm->setObjectName(QStringLiteral("page3Confirm"));
        sizePolicy.setHeightForWidth(page3Confirm->sizePolicy().hasHeightForWidth());
        page3Confirm->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(page3Confirm);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        labelMappedTo = new QLabel(page3Confirm);
        labelMappedTo->setObjectName(QStringLiteral("labelMappedTo"));
        sizePolicy.setHeightForWidth(labelMappedTo->sizePolicy().hasHeightForWidth());
        labelMappedTo->setSizePolicy(sizePolicy);
#ifndef QT_NO_TOOLTIP
        labelMappedTo->setToolTip(QStringLiteral(""));
#endif // QT_NO_TOOLTIP
        labelMappedTo->setText(QStringLiteral("(successfully mapped to message goes here)"));
        labelMappedTo->setWordWrap(true);

        verticalLayout->addWidget(labelMappedTo);

        labelNextHelp = new QLabel(page3Confirm);
        labelNextHelp->setObjectName(QStringLiteral("labelNextHelp"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(labelNextHelp->sizePolicy().hasHeightForWidth());
        labelNextHelp->setSizePolicy(sizePolicy2);
        labelNextHelp->setText(QStringLiteral("(html text)"));
        labelNextHelp->setWordWrap(true);

        verticalLayout->addWidget(labelNextHelp);

        midiOptions = new QGroupBox(page3Confirm);
        midiOptions->setObjectName(QStringLiteral("midiOptions"));
        sizePolicy.setHeightForWidth(midiOptions->sizePolicy().hasHeightForWidth());
        midiOptions->setSizePolicy(sizePolicy);
        gridLayout_2 = new QGridLayout(midiOptions);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        midiOptionSwitchMode = new QCheckBox(midiOptions);
        midiOptionSwitchMode->setObjectName(QStringLiteral("midiOptionSwitchMode"));

        gridLayout_2->addWidget(midiOptionSwitchMode, 1, 1, 1, 1);

        midiOptionSoftTakeover = new QCheckBox(midiOptions);
        midiOptionSoftTakeover->setObjectName(QStringLiteral("midiOptionSoftTakeover"));

        gridLayout_2->addWidget(midiOptionSoftTakeover, 1, 0, 1, 1);

        midiOptionInvert = new QCheckBox(midiOptions);
        midiOptionInvert->setObjectName(QStringLiteral("midiOptionInvert"));

        gridLayout_2->addWidget(midiOptionInvert, 2, 0, 1, 1);

        midiOptionSelectKnob = new QCheckBox(midiOptions);
        midiOptionSelectKnob->setObjectName(QStringLiteral("midiOptionSelectKnob"));

        gridLayout_2->addWidget(midiOptionSelectKnob, 2, 1, 1, 1);


        verticalLayout->addWidget(midiOptions);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(10);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        pushButtonRetry = new QPushButton(page3Confirm);
        pushButtonRetry->setObjectName(QStringLiteral("pushButtonRetry"));

        horizontalLayout->addWidget(pushButtonRetry);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        pushButtonLearnAnother = new QPushButton(page3Confirm);
        pushButtonLearnAnother->setObjectName(QStringLiteral("pushButtonLearnAnother"));

        horizontalLayout->addWidget(pushButtonLearnAnother);

        pushButtonClose_2 = new QPushButton(page3Confirm);
        pushButtonClose_2->setObjectName(QStringLiteral("pushButtonClose_2"));

        horizontalLayout->addWidget(pushButtonClose_2);


        verticalLayout->addLayout(horizontalLayout);

        stackedWidget->addWidget(page3Confirm);

        gridLayout->addWidget(stackedWidget, 0, 0, 1, 1);


        retranslateUi(DlgControllerLearning);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(DlgControllerLearning);
    } // setupUi

    void retranslateUi(QDialog *DlgControllerLearning)
    {
        DlgControllerLearning->setWindowTitle(QApplication::translate("DlgControllerLearning", "Controller Learning Wizard", 0));
        pushButtonStartLearn->setText(QApplication::translate("DlgControllerLearning", "Learn", 0));
        pushButtonClose->setText(QApplication::translate("DlgControllerLearning", "Close", 0));
        pushButtonChooseControl->setText(QApplication::translate("DlgControllerLearning", "Choose Control...", 0));
        label->setText(QApplication::translate("DlgControllerLearning", "Hints: If you're mapping a button or switch, only press or flip it once.  For knobs and sliders, move the control in both directions for best results. Make sure to touch one control at a time.", 0));
        progressBarWiggleFeedback->setFormat(QString());
        pushButtonCancelLearn->setText(QApplication::translate("DlgControllerLearning", "Cancel", 0));
        midiOptions->setTitle(QApplication::translate("DlgControllerLearning", "Advanced MIDI Options", 0));
#ifndef QT_NO_TOOLTIP
        midiOptionSwitchMode->setToolTip(QApplication::translate("DlgControllerLearning", "Switch mode interprets all messages for the control as button presses.", 0));
#endif // QT_NO_TOOLTIP
        midiOptionSwitchMode->setText(QApplication::translate("DlgControllerLearning", "Switch Mode", 0));
#ifndef QT_NO_TOOLTIP
        midiOptionSoftTakeover->setToolTip(QApplication::translate("DlgControllerLearning", "Ignores slider or knob movements until they are close to the internal value. This helps prevent unwanted extreme changes while mixing but can accidentally ignore intentional rapid movements.", 0));
#endif // QT_NO_TOOLTIP
        midiOptionSoftTakeover->setText(QApplication::translate("DlgControllerLearning", "Soft Takeover", 0));
#ifndef QT_NO_TOOLTIP
        midiOptionInvert->setToolTip(QApplication::translate("DlgControllerLearning", "Reverses the direction of the control.", 0));
#endif // QT_NO_TOOLTIP
        midiOptionInvert->setText(QApplication::translate("DlgControllerLearning", "Invert", 0));
#ifndef QT_NO_TOOLTIP
        midiOptionSelectKnob->setToolTip(QApplication::translate("DlgControllerLearning", "For jog wheels or infinite-scroll knobs. Interprets incoming messages in two's complement.", 0));
#endif // QT_NO_TOOLTIP
        midiOptionSelectKnob->setText(QApplication::translate("DlgControllerLearning", "Jog Wheel / Select Knob", 0));
        pushButtonRetry->setText(QApplication::translate("DlgControllerLearning", "Retry", 0));
        pushButtonLearnAnother->setText(QApplication::translate("DlgControllerLearning", "Learn Another", 0));
        pushButtonClose_2->setText(QApplication::translate("DlgControllerLearning", "Done", 0));
    } // retranslateUi

};

namespace Ui {
    class DlgControllerLearning: public Ui_DlgControllerLearning {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DLGCONTROLLERLEARNING_H
