/*
 * Copyright (c) 2011 Yubico AB
 * See the file COPYING for licence statement.
 *
 */

#include "staticpage.h"
#include "ui_staticpage.h"
#include "ui/helpbox.h"
#include "ui/confirmbox.h"
#include "scanedit.h"

#include <QDesktopServices>
#include "common.h"

StaticPage::StaticPage(QWidget *parent) :
        QStackedWidget(parent),
        ui(new Ui::StaticPage)
{
    ui->setupUi(this);

    m_ykConfig = 0;
    clearState();

    //Connect pages
    connectPages();

    //Connect help buttons
    connectHelpButtons();

    //Connect other signals and slots
    connect(YubiKeyFinder::getInstance(), SIGNAL(keyFound(bool, bool*)),
            this, SLOT(keyFound(bool, bool*)));

    connect(ui->quickResetBtn, SIGNAL(clicked()),
            this, SLOT(resetQuickPage()));
    connect(ui->advResetBtn, SIGNAL(clicked()),
            this, SLOT(resetAdvPage()));

    ui->quickResultsWidget->resizeColumnsToContents();
    ui->advResultsWidget->resizeColumnsToContents();
}

StaticPage::~StaticPage() {
    if(m_ykConfig != 0) {
        delete m_ykConfig;
        m_ykConfig = 0;
    }
    delete ui;
}

/*
 Common
*/

void StaticPage::connectPages() {
    //Map the values of the navigation buttons with the indexes of
    //the stacked widget

    //Create a QMapper
    QSignalMapper *mapper = new QSignalMapper(this);

    //Connect the clicked signal with the QSignalMapper
    connect(ui->quickBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->quickBackBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    //connect(ui->quickUploadBtn, SIGNAL(clicked()), mapper, SLOT(map()));

    connect(ui->advBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->advBackBtn, SIGNAL(clicked()), mapper, SLOT(map()));

    //Set a value for each button
    mapper->setMapping(ui->quickBtn, Page_Quick);
    mapper->setMapping(ui->quickBackBtn, Page_Base);
    //mapper->setMapping(ui->quickUploadBtn, Page_Upload);

    mapper->setMapping(ui->advBtn, Page_Advanced);
    mapper->setMapping(ui->advBackBtn, Page_Base);

    //Connect the mapper to the widget
    //The mapper will set a value to each button and
    //set that value to the widget
    //connect(pageMapper, SIGNAL(mapped(int)), this, SLOT(setCurrentIndex(int)));
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(setCurrentPage(int)));

    //Set the current page
    m_currentPage = 0;
    setCurrentIndex(Page_Base);
}

void StaticPage::setCurrentPage(int pageIndex) {
    //Page changed...

    m_currentPage = pageIndex;

    switch(pageIndex){
    case Page_Quick:
        resetQuickPage();
        break;
    case Page_Advanced:
        resetAdvPage();
        break;
    }

    setCurrentIndex(pageIndex);

    //Clear state
    m_keysProgrammedCtr = 0;
    clearState();
}

void StaticPage::connectHelpButtons() {
    //Map the values of the help buttons

    //Create a QMapper
    QSignalMapper *mapper = new QSignalMapper(this);

    //Connect the clicked signal with the QSignalMapper
    connect(ui->quickConfigHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->quickConfigProtectionHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->quickStaticScanCodeHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));

    connect(ui->advConfigHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->advParamGenSchemeHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->advConfigProtectionHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->advPubIdHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->advPvtIdHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->advSecretKeyHelpBtn, SIGNAL(clicked()), mapper, SLOT(map()));

    //Set a value for each button
    mapper->setMapping(ui->quickConfigHelpBtn, HelpBox::Help_ConfigurationSlot);
    mapper->setMapping(ui->quickConfigProtectionHelpBtn, HelpBox::Help_ConfigurationProtection);
    mapper->setMapping(ui->quickStaticScanCodeHelpBtn, HelpBox::Help_StaticScanCode);

    mapper->setMapping(ui->advConfigHelpBtn, HelpBox::Help_ConfigurationSlot);
    mapper->setMapping(ui->advParamGenSchemeHelpBtn, HelpBox::Help_ParameterGeneration);
    mapper->setMapping(ui->advConfigProtectionHelpBtn, HelpBox::Help_ConfigurationProtection);
    mapper->setMapping(ui->advPubIdHelpBtn, HelpBox::Help_PublicID);
    mapper->setMapping(ui->advPvtIdHelpBtn, HelpBox::Help_PrivateID);
    mapper->setMapping(ui->advSecretKeyHelpBtn, HelpBox::Help_SecretKey);

    //Connect the mapper
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(helpBtn_pressed(int)));
}

void StaticPage::helpBtn_pressed(int helpIndex) {
    HelpBox help(this);
    help.setHelpIndex((HelpBox::Help)helpIndex);
    help.exec();
}

void StaticPage::keyFound(bool found, bool* featuresMatrix) {
    if(found) {
        if(m_state == State_Initial) {
            ui->quickWriteConfigBtn->setEnabled(true);
            ui->advWriteConfigBtn->setEnabled(true);

            if(!featuresMatrix[YubiKeyFinder::Feature_MultipleConfigurations]) {
                ui->quickConfigSlot2Radio->setEnabled(false);
                ui->advConfigSlot2Radio->setEnabled(false);
            } else {
                ui->quickConfigSlot2Radio->setEnabled(true);
                ui->advConfigSlot2Radio->setEnabled(true);
            }

            if(!featuresMatrix[YubiKeyFinder::Feature_ShortTicket]) {
                ui->advStaticLen16Radio->setEnabled(false);
                ui->advStaticLen32Radio->setChecked(true);
            }

            if(!featuresMatrix[YubiKeyFinder::Feature_StrongPwd]) {
                ui->advStrongPw1Check->setEnabled(false);
                ui->advStrongPw2Check->setEnabled(false);
                ui->advStrongPw3Check->setEnabled(false);
            }

            if(!featuresMatrix[YubiKeyFinder::Feature_ScanCodeMode]) {
                ui->quickBtn->setEnabled(false);

                ui->quickConfigBox->setEnabled(false);
                ui->quickProgramMulKeysBox->setEnabled(false);
                ui->quickConfigProtectionBox->setEnabled(false);
                ui->quickKeyParamsBox->setEnabled(false);

                ui->quickWriteConfigBtn->setEnabled(false);
            }

            if(!featuresMatrix[YubiKeyFinder::Feature_StaticPassword]) {
                this->setEnabled(false);
            }
        } else if(m_state >= State_Programming_Multiple) {
            if(this->currentIndex() == Page_Quick) {
                if(m_state == State_Programming_Multiple) {
                    ui->quickWriteConfigBtn->setEnabled(true);
                } else {
                    writeQuickConfig();
                }
            } else {
                if(m_state == State_Programming_Multiple) {
                    ui->advWriteConfigBtn->setEnabled(true);
                } else {
                    writeAdvConfig();
                }
            }
        }
    } else {
        ui->quickBtn->setEnabled(true);
        ui->quickWriteConfigBtn->setEnabled(false);
        ui->advWriteConfigBtn->setEnabled(false);

        if(m_state == State_Initial) {
            ui->quickConfigSlot2Radio->setEnabled(true);
            ui->advConfigSlot2Radio->setEnabled(true);

            ui->advStaticLen16Radio->setEnabled(true);

            ui->advStrongPw1Check->setEnabled(true);
            ui->advStrongPw2Check->setEnabled(true);
            ui->advStrongPw3Check->setEnabled(true);

            ui->quickConfigBox->setEnabled(true);
            ui->quickProgramMulKeysBox->setEnabled(true);
            ui->quickConfigProtectionBox->setEnabled(true);
            ui->quickKeyParamsBox->setEnabled(true);

            this->setEnabled(true);
        } else if(m_state >= State_Programming_Multiple) {
            if(m_keysProgrammedCtr > 0 && !m_ready) {
                if(this->currentIndex() == Page_Quick) {
                    changeQuickConfigParams();
                } else {
                    changeAdvConfigParams();
                }
            }
        }
    }
}

void StaticPage::clearState() {
    m_state = State_Initial;
    m_ready = true;

    if(m_ykConfig != 0) {
        delete m_ykConfig;
        m_ykConfig = 0;
    }
}

/*
 Quick Page handling
*/
void StaticPage::resetQuickPage() {
    if(ui->quickConfigSlot1Radio->isChecked()) {
        ui->quickConfigSlot2Radio->setChecked(true);
    }

    ui->quickAutoProgramKeysCheck->setChecked(false);
    ui->quickProgramMulKeysBox->setChecked(false);

    ui->quickConfigProtectionCombo->setCurrentIndex(0);

    ui->quickStaticLenTxt->setText("0");
    ui->quickStaticTxt->clear();
    ui->quickInsertTabBtn->setEnabled(true);
    ui->quickClearBtn->setEnabled(false);

    ui->quickStopBtn->setEnabled(false);
    ui->quickResetBtn->setEnabled(false);
}

void StaticPage::freezeQuickPage(bool freeze) {
    bool disable = !freeze;
    ui->quickConfigBox->setEnabled(disable);
    ui->quickProgramMulKeysBox->setEnabled(disable);
    ui->quickConfigProtectionBox->setEnabled(disable);
    ui->quickKeyParamsBox->setEnabled(disable);

    ui->quickWriteConfigBtn->setEnabled(disable);
    ui->quickStopBtn->setEnabled(!disable);
    ui->quickResetBtn->setEnabled(disable);
    ui->quickBackBtn->setEnabled(disable);
}

void StaticPage::on_quickConfigProtectionCombo_currentIndexChanged(int index) {
    switch(index) {
    case CONFIG_PROTECTION_DISABLED:
        ui->quickCurrentAccessCodeTxt->clear();
        ui->quickCurrentAccessCodeTxt->setEnabled(false);

        ui->quickNewAccessCodeTxt->clear();
        ui->quickNewAccessCodeTxt->setEnabled(false);
        break;
    case CONFIG_PROTECTION_ENABLE:
        ui->quickCurrentAccessCodeTxt->clear();
        ui->quickCurrentAccessCodeTxt->setEnabled(false);

        on_quickNewAccessCodeTxt_editingFinished();
        ui->quickNewAccessCodeTxt->setEnabled(true);
        break;
    case CONFIG_PROTECTION_DISABLE:
    case CONFIG_PROTECTION_ENABLED:
        on_quickCurrentAccessCodeTxt_editingFinished();
        ui->quickCurrentAccessCodeTxt->setEnabled(true);

        ui->quickNewAccessCodeTxt->clear();
        ui->quickNewAccessCodeTxt->setEnabled(false);
        break;
    case CONFIG_PROTECTION_CHANGE:
        on_quickCurrentAccessCodeTxt_editingFinished();
        ui->quickCurrentAccessCodeTxt->setEnabled(true);

        on_quickNewAccessCodeTxt_editingFinished();
        ui->quickNewAccessCodeTxt->setEnabled(true);
        break;
    }
}

void StaticPage::on_quickCurrentAccessCodeTxt_editingFinished() {
    QString txt = ui->quickCurrentAccessCodeTxt->text();
    YubiKeyUtil::qstrClean(&txt, (size_t)ACC_CODE_SIZE * 2);
    ui->quickCurrentAccessCodeTxt->setText(txt);
}

void StaticPage::on_quickNewAccessCodeTxt_editingFinished() {
    QString txt = ui->quickNewAccessCodeTxt->text();
    YubiKeyUtil::qstrClean(&txt, (size_t)ACC_CODE_SIZE * 2);
    ui->quickNewAccessCodeTxt->setText(txt);
}

void StaticPage::on_quickHideParams_clicked(bool checked) {
    if(checked) {
        ui->quickStaticTxt->setEchoMode(QLineEdit::Password);
    } else {
        ui->quickStaticTxt->setEchoMode(QLineEdit::Normal);
    }
}

void StaticPage::on_quickStaticTxt_textChanged(const QString &txt) {
    int len = ui->quickStaticTxt->len();

    ui->quickStaticLenTxt->setText(QString::number(len));
    if(len >= MAX_SCAN_EDIT_SIZE) {
        ui->quickInsertTabBtn->setEnabled(false);
    } else {
        ui->quickInsertTabBtn->setEnabled(true);
    }

    ui->quickClearBtn->setEnabled(len > 0);
}

void StaticPage::on_quickInsertTabBtn_clicked() {
    ui->quickStaticTxt->injectScanCode(0x2b);
    ui->quickStaticTxt->setFocus();
}

void StaticPage::on_quickClearBtn_clicked() {
    ui->quickStaticTxt->clearScanCodeText();
    ui->quickStaticTxt->clear();
    ui->quickStaticTxt->setFocus();
}

void StaticPage::on_quickWriteConfigBtn_clicked() {
    emit showStatusMessage(NULL, -1);

    if(!ui->quickProgramMulKeysBox->isChecked()) {
        m_keysProgrammedCtr = 0;
    }

    //Validate settings
    if(!validateQuickSettings()) {
        return;
    }

    clearState();

    freezeQuickPage(true);

    // Change state
    if(ui->quickProgramMulKeysBox->isChecked()) {
        if(ui->quickAutoProgramKeysCheck->isChecked()) {
            m_keysProgrammedCtr = 0;
            m_state = State_Programming_Multiple_Auto;
        } else {
            m_state = State_Programming_Multiple;
        }
    } else {
        m_keysProgrammedCtr = 0;
        m_state = State_Programming;
    }

    writeQuickConfig();
}

void StaticPage::on_quickStopBtn_clicked() {
    ui->quickStopBtn->setEnabled(false);
    m_state = State_Initial;
    stopQuickConfigWritting();
}

void StaticPage::stopQuickConfigWritting() {
    qDebug() << "Stopping quick confgiuration writing...";

    if(m_state >= State_Programming_Multiple) {
        ui->quickStopBtn->setEnabled(true);
        return;
    }

    m_keysProgrammedCtr = 0;
    clearState();

    freezeQuickPage(false);
    ui->quickResetBtn->setEnabled(true);
}

void StaticPage::changeQuickConfigParams() {
    m_ready = true;
}

bool StaticPage::validateQuickSettings() {
    if(!(ui->quickConfigSlot1Radio->isChecked() ||
         ui->quickConfigSlot2Radio->isChecked())) {
        emit showStatusMessage(ERR_CONF_SLOT_NOT_SELECTED, 1);
        return false;
    }

    //Check if configuration slot 1 is being programmed
    if (ui->quickStaticLenTxt->text().toInt() == 0) {
        QMessageBox::critical(this, ERR, WARN_EMPTY_PASS);
        return false;
    }

    QSettings settings;

    //Check if configuration slot 1 is being programmed
    if (!settings.value(SG_OVERWRITE_CONF_SLOT1).toBool() &&
        ui->quickConfigSlot1Radio->isChecked() &&
        m_keysProgrammedCtr == 0) {
        //Confirm from client
        ConfirmBox confirm(this);
        confirm.setConfirmIndex(ConfirmBox::Confirm_ConfigurationSlot);
        int ret = confirm.exec();

        switch (ret) {
        case 1:     //Yes
            break;
        default:    //No
            return false;
        }
    }

    //Check if logging is disabled and
    //configuration protection is being enabled
    if(!settings.value(SG_ENABLE_CONF_PROTECTION).toBool() &&
       !YubiKeyLogger::isLogging() &&
       ui->quickConfigProtectionCombo->currentIndex() == CONFIG_PROTECTION_ENABLE) {
        //Confirm from client
        ConfirmBox confirm(this);
        confirm.setConfirmIndex(ConfirmBox::Confirm_ConfigurationProtection);
        int ret = confirm.exec();

        switch (ret) {
        case 1:     //Yes
            break;
        default:    //No
            return false;
        }
    }
    return true;
}

void StaticPage::writeQuickConfig() {
    qDebug() << "Writing configuration...";

    //Disable stop button while configuration is being written
    ui->quickStopBtn->setEnabled(false);

    //Write configuration
    if(m_ykConfig != 0) {
        qDebug() << "ykConfig destroyed";
        delete m_ykConfig;
        m_ykConfig = 0;
    }
    m_ykConfig = new YubiKeyConfig();

    //Programming mode...
    m_ykConfig->setProgrammingMode(YubiKeyConfig::Mode_Static);

    //Configuration slot...
    int configSlot = 1;
    if(ui->quickConfigSlot2Radio->isChecked()) {
        configSlot = 2;
    }
    m_ykConfig->setConfigSlot(configSlot);

    //Parameters...
    QString staticTxt = ui->quickStaticTxt->scanCodeText();
    YubiKeyUtil::qstrClean(&staticTxt, 0);

    QString pubIdTxt("");
    QString pvtIdTxt("");
    QString secretKeyTxt("");

    int staticLen = staticTxt.length() / 2;
    int len = staticLen;

    // If longer than size of fixed, fill up UID and key
    if(staticLen > FIXED_SIZE) {
        staticLen -= UID_SIZE;

        if(staticLen > FIXED_SIZE) {
            staticLen -=  KEY_SIZE;

            secretKeyTxt = staticTxt.mid((len - KEY_SIZE) * 2, KEY_SIZE * 2);
            pvtIdTxt = staticTxt.mid((len - UID_SIZE - KEY_SIZE) * 2, UID_SIZE * 2);
        } else {
            pvtIdTxt = staticTxt.mid((len - UID_SIZE) * 2, UID_SIZE * 2);
        }
    }

    qDebug() << "staticLen:" << staticLen;

    // Keep what's left in the fixed part
    pubIdTxt = staticTxt.mid(0, staticLen * 2);

    qDebug() << "pubIdTxt:" << pubIdTxt;
    qDebug() << "pvtIdTxt:" << pvtIdTxt;
    qDebug() << "secretKeyTxt:" << secretKeyTxt;

    //Public ID...
    if(!pubIdTxt.isEmpty()) {
        m_ykConfig->setPubIdInHex(true);
        m_ykConfig->setPubIdTxt(pubIdTxt);
    }

    //Private ID...
    if(!pvtIdTxt.isEmpty()) {
        m_ykConfig->setPvtIdTxt(pvtIdTxt);
    }

    //Secret Key...
    if(!secretKeyTxt.isEmpty()) {
        m_ykConfig->setSecretKeyTxt(secretKeyTxt);
    }

    //Configuration protection...
    //Current Access Code...
    m_ykConfig->setCurrentAccessCodeTxt(ui->quickCurrentAccessCodeTxt->text());

    //New Access Code...
    if(ui->quickConfigProtectionCombo->currentIndex()
        == CONFIG_PROTECTION_DISABLE){
        m_ykConfig->setNewAccessCodeTxt(ACCESS_CODE_DEFAULT);
    } else {
        m_ykConfig->setNewAccessCodeTxt(ui->quickNewAccessCodeTxt->text());
    }

    //Static Options...
    m_ykConfig->setShortTicket(true);

    //Write
    connect(YubiKeyWriter::getInstance(), SIGNAL(configWritten(bool, const QString &)),
            this, SLOT(quickConfigWritten(bool, const QString &)));

    YubiKeyWriter::getInstance()->writeConfig(m_ykConfig);
}

void StaticPage::quickConfigWritten(bool written, const QString &msg) {
    disconnect(YubiKeyWriter::getInstance(), SIGNAL(configWritten(bool, const QString &)),
               this, SLOT(quickConfigWritten(bool, const QString &)));

    QString message;

    if(written) {
        int staticLen = ui->quickStaticLenTxt->text().toInt();

        QString keyDetail = tr(" (Password Length: %1 chars)").arg(staticLen);

        if(m_state == State_Programming){
            message = KEY_CONFIGURED.arg(keyDetail);
        } else {
            message = tr("%1. %2").arg(KEY_CONFIGURED.arg(keyDetail)).arg(REMOVE_KEY);
        }

        showStatusMessage(message, 0);

        message = KEY_CONFIGURED.arg("");
    } else {
        qDebug() << "Configuration could not be written....";

        message = msg;
    }

    quickUpdateResults(written, message);

    m_ready = false;
    stopQuickConfigWritting();
}

void StaticPage::quickUpdateResults(bool written, const QString &msg) {
    int row = 0;

    ui->quickResultsWidget->insertRow(row);

    //Sr. No....
    QTableWidgetItem *srnoItem = new QTableWidgetItem(
            tr("%1").arg(ui->quickResultsWidget->rowCount()));
    if(written) {
        srnoItem->setIcon(QIcon(QPixmap(tr(":/res/images/tick.png"))));
    } else {
        srnoItem->setIcon(QIcon(QPixmap(tr(":/res/images/cross.png"))));
    }
    srnoItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->quickResultsWidget->setItem(row, 0, srnoItem);


    //Password Len....
    int staticLen = ui->quickStaticLenTxt->text().toInt();
    QTableWidgetItem *lenItem = new QTableWidgetItem(
            tr("%1").arg(QString::number(staticLen)));
    ui->quickResultsWidget->setItem(row, 1, lenItem);


    //Status...
    QTableWidgetItem *statusItem = new QTableWidgetItem(
            tr("%1").arg(msg));
    statusItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->quickResultsWidget->setItem(row, 2, statusItem);


    //Timestamp...
    QDateTime timstamp = QDateTime::currentDateTime();
    QTableWidgetItem *timeItem = new QTableWidgetItem(
            tr("%1").arg(timstamp.toString(Qt::SystemLocaleShortDate)));
    timeItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->quickResultsWidget->setItem(row, 3, timeItem);


    ui->quickResultsWidget->resizeColumnsToContents();
    ui->quickResultsWidget->resizeRowsToContents();
}

/*
 Advanced Page handling
*/

void StaticPage::resetAdvPage() {
    if(ui->advConfigSlot1Radio->isChecked()) {
        ui->advConfigSlot2Radio->setChecked(true);
    }

    ui->advConfigParamsCombo->setCurrentIndex(SCHEME_FIXED);
    ui->advAutoProgramKeysCheck->setChecked(false);
    ui->advProgramMulKeysBox->setChecked(false);

    ui->advConfigProtectionCombo->setCurrentIndex(0);

    ui->advStaticLen32Radio->setChecked(true);
    int minStaticLen = ui->advStaticLenBox->minimum();
    ui->advStaticLenBox->setValue(minStaticLen);

    enablePubId(false);

    ui->advPvtIdTxt->clear();
    on_advPvtIdTxt_editingFinished();

    ui->advSecretKeyTxt->clear();
    on_advSecretKeyTxt_editingFinished();

    ui->advStrongPw1Check->setChecked(false);
    ui->advStrongPw2Check->setChecked(false);
    ui->advStrongPw3Check->setChecked(false);

    ui->advStopBtn->setEnabled(false);
    ui->advResetBtn->setEnabled(false);
}

void StaticPage::freezeAdvPage(bool freeze) {
    bool disable = !freeze;
    ui->advConfigBox->setEnabled(disable);
    ui->advProgramMulKeysBox->setEnabled(disable);
    ui->advConfigProtectionBox->setEnabled(disable);
    ui->advKeyParamsBox->setEnabled(disable);

    ui->advWriteConfigBtn->setEnabled(disable);
    ui->advStopBtn->setEnabled(!disable);
    ui->advResetBtn->setEnabled(disable);
    ui->advBackBtn->setEnabled(disable);
}

void StaticPage::on_advProgramMulKeysBox_clicked(bool checked) {
    if(checked) {
        changeAdvConfigParams();
    }
}

void StaticPage::on_advConfigParamsCombo_currentIndexChanged(int index) {
    changeAdvConfigParams();
}

void StaticPage::on_advConfigProtectionCombo_currentIndexChanged(int index) {
    switch(index) {
    case CONFIG_PROTECTION_DISABLED:
        ui->advCurrentAccessCodeTxt->clear();
        ui->advCurrentAccessCodeTxt->setEnabled(false);

        ui->advNewAccessCodeTxt->clear();
        ui->advNewAccessCodeTxt->setEnabled(false);
        break;
    case CONFIG_PROTECTION_ENABLE:
        ui->advCurrentAccessCodeTxt->clear();
        ui->advCurrentAccessCodeTxt->setEnabled(false);

        on_advNewAccessCodeTxt_editingFinished();
        ui->advNewAccessCodeTxt->setEnabled(true);
        break;
    case CONFIG_PROTECTION_DISABLE:
    case CONFIG_PROTECTION_ENABLED:
        on_advCurrentAccessCodeTxt_editingFinished();
        ui->advCurrentAccessCodeTxt->setEnabled(true);

        ui->advNewAccessCodeTxt->clear();
        ui->advNewAccessCodeTxt->setEnabled(false);
        break;
    case CONFIG_PROTECTION_CHANGE:
        on_advCurrentAccessCodeTxt_editingFinished();
        ui->advCurrentAccessCodeTxt->setEnabled(true);

        on_advNewAccessCodeTxt_editingFinished();
        ui->advNewAccessCodeTxt->setEnabled(true);
        break;
    }
}

void StaticPage::on_advCurrentAccessCodeTxt_editingFinished() {
    QString txt = ui->advCurrentAccessCodeTxt->text();
    YubiKeyUtil::qstrClean(&txt, (size_t)ACC_CODE_SIZE * 2);
    ui->advCurrentAccessCodeTxt->setText(txt);
}

void StaticPage::on_advNewAccessCodeTxt_editingFinished() {
    QString txt = ui->advNewAccessCodeTxt->text();
    YubiKeyUtil::qstrClean(&txt, (size_t)ACC_CODE_SIZE * 2);
    ui->advNewAccessCodeTxt->setText(txt);
}

void StaticPage::on_advStaticLenBox_valueChanged(int value) {
    int len = value / 2 - KEY_SIZE;
    if(len > 0) {
        enablePubId(true);

        QString modhexMask("");
        for(int i = 1; i <= len; i++) {
            modhexMask.append("nn ");
        }
        ui->advPubIdTxt->setInputMask(modhexMask);
        on_advPubIdTxt_editingFinished();
    } else {
        enablePubId(false);
    }
}

void StaticPage::on_advStaticLen16Radio_clicked(bool checked) {
    ui->advStaticLenBox->setEnabled(!checked);
    enablePubId(!checked);
}

void StaticPage::on_advStaticLen32Radio_clicked(bool checked) {
    ui->advStaticLenBox->setEnabled(true);
    on_advStaticLenBox_valueChanged(ui->advStaticLenBox->value());
}

void StaticPage::enablePubId(bool enable) {
    if(!enable) {
        ui->advPubIdTxt->clear();
        ui->advPubIdTxt->inputMask();
    } else {
        on_advPubIdTxt_editingFinished();
    }
    ui->advPubIdTxt->setEnabled(enable);
    ui->advPubIdGenerateBtn->setEnabled(enable);
}

void StaticPage::on_advPubIdTxt_editingFinished() {
    QString txt = ui->advPubIdTxt->text();
    size_t len = ui->advPubIdTxt->maxLength() / 3;
    YubiKeyUtil::qstrModhexClean(&txt, (size_t)len * 2);

    ui->advPubIdTxt->setText(txt);
    len = txt.length();
    ui->advPubIdTxt->setCursorPosition(len + len/2);
}

void StaticPage::on_advPubIdGenerateBtn_clicked() {
    size_t len = ui->advPubIdTxt->maxLength() / 3;
    QString txt = YubiKeyUtil::generateRandomModhex((size_t)len * 2);
    ui->advPubIdTxt->setText(txt);
}

void StaticPage::on_advPvtIdTxt_editingFinished() {
    QString txt = ui->advPvtIdTxt->text();
    YubiKeyUtil::qstrClean(&txt, (size_t)UID_SIZE * 2);
    ui->advPvtIdTxt->setText(txt);
}

void StaticPage::on_advPvtIdGenerateBtn_clicked() {
    ui->advPvtIdTxt->setText(
            YubiKeyUtil::generateRandomHex((size_t)UID_SIZE * 2));
}

void StaticPage::on_advSecretKeyTxt_editingFinished() {
    QString txt = ui->advSecretKeyTxt->text();
    YubiKeyUtil::qstrClean(&txt, (size_t)KEY_SIZE * 2);
    ui->advSecretKeyTxt->setText(txt);
}

void StaticPage::on_advSecretKeyGenerateBtn_clicked() {
    ui->advSecretKeyTxt->setText(
            YubiKeyUtil::generateRandomHex((size_t)KEY_SIZE * 2));
}

void StaticPage::on_advStrongPw2Check_stateChanged(int state) {
    if(state == 0) {
        ui->advStrongPw3Check->setChecked(false);
    }
}

void StaticPage::on_advStrongPw3Check_stateChanged(int state) {
    if(!ui->advStrongPw2Check->isChecked() && state > 0) {
        ui->advStrongPw2Check->setChecked(true);
    }
}

void StaticPage::on_advWriteConfigBtn_clicked() {
    emit showStatusMessage(NULL, -1);

    if(!ui->advProgramMulKeysBox->isChecked()) {
        m_keysProgrammedCtr = 0;
    }

    //Validate settings
    if(!validateAdvSettings()) {
        return;
    }

    clearState();

    freezeAdvPage(true);

    // Change state
    if(ui->advProgramMulKeysBox->isChecked()) {
        if(ui->advAutoProgramKeysCheck->isChecked()) {
            m_keysProgrammedCtr = 0;
            m_state = State_Programming_Multiple_Auto;
        } else {
            m_state = State_Programming_Multiple;
        }
    } else {
        m_keysProgrammedCtr = 0;
        m_state = State_Programming;
    }

    writeAdvConfig();
}

void StaticPage::on_advStopBtn_clicked() {
    ui->advStopBtn->setEnabled(false);
    m_state = State_Initial;
    stopAdvConfigWritting();
}

void StaticPage::stopAdvConfigWritting() {
    qDebug() << "Stopping adv confgiuration writing...";

    if(m_state >= State_Programming_Multiple) {
        ui->advStopBtn->setEnabled(true);
        return;
    }

    m_keysProgrammedCtr = 0;
    clearState();

    freezeAdvPage(false);
    ui->advResetBtn->setEnabled(true);
}

void StaticPage::changeAdvConfigParams() {
    int index = ui->advConfigParamsCombo->currentIndex();

    if(index == SCHEME_FIXED) {
        m_ready = true;
        return;
    }

    int idScheme = GEN_SCHEME_FIXED;
    int secretScheme = GEN_SCHEME_FIXED;

    switch(index) {
    case SCHEME_INCR_ID_RAND_SECRET:
        //Increment IDs only if in programming mode
        if(m_state != State_Initial) {
            idScheme = GEN_SCHEME_INCR;
        }
        secretScheme = GEN_SCHEME_RAND;
        break;

    case SCHEME_RAND_ALL:
        idScheme = GEN_SCHEME_RAND;
        secretScheme = GEN_SCHEME_RAND;
        break;
    }

    //Public ID...
    int len = ui->advStaticLenBox->value() / 2 - KEY_SIZE;
    QString pubIdTxt = YubiKeyUtil::getNextModhex(
            len * 2,
            ui->advPubIdTxt->text(), idScheme);

    ui->advPubIdTxt->setText(pubIdTxt);
    on_advPubIdTxt_editingFinished();

    //Private ID...
    QString pvtIdTxt = YubiKeyUtil::getNextHex(
            UID_SIZE * 2,
            ui->advPvtIdTxt->text(), idScheme);
    ui->advPvtIdTxt->setText(pvtIdTxt);

    //Secret Key...
    QString secretKeyTxt = YubiKeyUtil::getNextHex(
            KEY_SIZE * 2,
            ui->advSecretKeyTxt->text(), secretScheme);
    ui->advSecretKeyTxt->setText(secretKeyTxt);

    m_ready = true;
}

bool StaticPage::validateAdvSettings() {
    if(!(ui->advConfigSlot1Radio->isChecked() ||
         ui->advConfigSlot2Radio->isChecked())) {
        emit showStatusMessage(ERR_CONF_SLOT_NOT_SELECTED, 1);
        return false;
    }

    QSettings settings;

    //Check if configuration slot 1 is being programmed
    if (!settings.value(SG_OVERWRITE_CONF_SLOT1).toBool() &&
        ui->advConfigSlot1Radio->isChecked() &&
        m_keysProgrammedCtr == 0) {
        //Confirm from client
        ConfirmBox confirm(this);
        confirm.setConfirmIndex(ConfirmBox::Confirm_ConfigurationSlot);
        int ret = confirm.exec();

        switch (ret) {
        case 1:     //Yes
            break;
        default:    //No
            return false;
        }
    }

    //Check if logging is disabled and
    //configuration protection is being enabled
    if(!settings.value(SG_ENABLE_CONF_PROTECTION).toBool() &&
       !YubiKeyLogger::isLogging() &&
       ui->advConfigProtectionCombo->currentIndex() == CONFIG_PROTECTION_ENABLE) {
        //Confirm from client
        ConfirmBox confirm(this);
        confirm.setConfirmIndex(ConfirmBox::Confirm_ConfigurationProtection);
        int ret = confirm.exec();

        switch (ret) {
        case 1:     //Yes
            break;
        default:    //No
            return false;
        }
    }
    return true;
}

void StaticPage::writeAdvConfig() {
    qDebug() << "Writing configuration...";

    //Disable stop button while configuration is being written
    ui->advStopBtn->setEnabled(false);

    //Write configuration
    if(m_ykConfig != 0) {
        qDebug() << "ykConfig destroyed";
        delete m_ykConfig;
        m_ykConfig = 0;
    }
    m_ykConfig = new YubiKeyConfig();

    //Programming mode...
    m_ykConfig->setProgrammingMode(YubiKeyConfig::Mode_Static);

    //Configuration slot...
    int configSlot = 1;
    if(ui->advConfigSlot2Radio->isChecked()) {
        configSlot = 2;
    }
    m_ykConfig->setConfigSlot(configSlot);

    //Public ID...
    m_ykConfig->setPubIdTxt(ui->advPubIdTxt->text());

    //Private ID...
    m_ykConfig->setPvtIdTxt(ui->advPvtIdTxt->text());

    //Secret Key...
    m_ykConfig->setSecretKeyTxt(ui->advSecretKeyTxt->text());

    //Configuration protection...
    //Current Access Code...
    m_ykConfig->setCurrentAccessCodeTxt(ui->advCurrentAccessCodeTxt->text());

    //New Access Code...
    if(ui->advConfigProtectionCombo->currentIndex()
        == CONFIG_PROTECTION_DISABLE){
        m_ykConfig->setNewAccessCodeTxt(ACCESS_CODE_DEFAULT);
    } else {
        m_ykConfig->setNewAccessCodeTxt(ui->advNewAccessCodeTxt->text());
    }

    //Static Options...
    m_ykConfig->setStaticTicket(true);
    if(ui->advStaticLen16Radio->isChecked()) {
        m_ykConfig->setShortTicket(true);
    }

    if(YubiKeyFinder::getInstance()->checkFeatureSupport(
            YubiKeyFinder::Feature_StrongPwd)) {

        if(ui->advStrongPw1Check->isChecked()) {
            m_ykConfig->setStrongPw1(true);
        }
        if(ui->advStrongPw2Check->isChecked()) {
            m_ykConfig->setStrongPw2(true);
        }
        if(ui->advStrongPw3Check->isChecked()) {
            m_ykConfig->setSendRef(true);
        }
    }

    QSettings settings;
    m_ykConfig->setManUpdate(settings.value(SG_MAN_UPDATE).toBool());

    //Write
    connect(YubiKeyWriter::getInstance(), SIGNAL(configWritten(bool, const QString &)),
            this, SLOT(advConfigWritten(bool, const QString &)));

    YubiKeyWriter::getInstance()->writeConfig(m_ykConfig);
}

void StaticPage::advConfigWritten(bool written, const QString &msg) {
    disconnect(YubiKeyWriter::getInstance(), SIGNAL(configWritten(bool, const QString &)),
               this, SLOT(advConfigWritten(bool, const QString &)));

    QString message;

    if(written && m_ykConfig != 0) {
        qDebug() << "Configuration written....";

        m_keysProgrammedCtr++;

        int staticLen = 0;
        if(ui->advStaticLen16Radio->isChecked()) {
            staticLen = 16;
        } else {
            staticLen = ui->advStaticLenBox->value();
        }

        QString keyDetail = tr(" (Password Length: %1 chars)").arg(staticLen);

        if(m_state == State_Programming){
            message = KEY_CONFIGURED.arg(keyDetail);
        } else {
            message = tr("%1. %2").arg(KEY_CONFIGURED.arg(keyDetail)).arg(REMOVE_KEY);
        }

        showStatusMessage(message, 0);

        message = KEY_CONFIGURED.arg("");
    } else {
        qDebug() << "Configuration could not be written....";

        message = msg;
    }

    advUpdateResults(written, message);

    m_ready = false;
    stopAdvConfigWritting();
}

void StaticPage::advUpdateResults(bool written, const QString &msg) {
    int row = 0;

    ui->advResultsWidget->insertRow(row);

    //Sr. No....
    QTableWidgetItem *srnoItem = new QTableWidgetItem(
            tr("%1").arg(ui->advResultsWidget->rowCount()));
    if(written) {
        srnoItem->setIcon(QIcon(QPixmap(tr(":/res/images/tick.png"))));
    } else {
        srnoItem->setIcon(QIcon(QPixmap(tr(":/res/images/cross.png"))));
    }
    srnoItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->advResultsWidget->setItem(row, 0, srnoItem);

    //Password Len....
    int staticLen = 0;
    if(ui->advStaticLen16Radio->isChecked()) {
        staticLen = 16;
    } else {
        staticLen = ui->advStaticLenBox->value();
    }
    QTableWidgetItem *lenItem = new QTableWidgetItem(
            tr("%1").arg(QString::number(staticLen)));
    ui->advResultsWidget->setItem(row, 1, lenItem);


    //Public ID...
    QString pubId;
    if(m_ykConfig != 0 && m_ykConfig->pubIdTxt().length() > 0) {
        pubId = m_ykConfig->pubIdTxt();
    } else {
        pubId = NA;
    }
    QTableWidgetItem *idItem = new QTableWidgetItem(
            tr("%1").arg(pubId));

    idItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->advResultsWidget->setItem(row, 2, idItem);


    //Status...
    QTableWidgetItem *statusItem = new QTableWidgetItem(
            tr("%1").arg(msg));
    statusItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->advResultsWidget->setItem(row, 3, statusItem);


    //Timestamp...
    QDateTime timstamp = QDateTime::currentDateTime();
    QTableWidgetItem *timeItem = new QTableWidgetItem(
            tr("%1").arg(timstamp.toString(Qt::SystemLocaleShortDate)));
    timeItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->advResultsWidget->setItem(row, 4, timeItem);


    ui->advResultsWidget->resizeColumnsToContents();
    ui->advResultsWidget->resizeRowsToContents();
}
