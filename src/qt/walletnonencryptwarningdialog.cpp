// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The BlocknetDX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/walletnonencryptwarningdialog.h"
#include "ui_walletnonencryptwarningdialog.h"

#include "guiutil.h"


WalletNonEncryptWarningDialog::WalletNonEncryptWarningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WalletNonEncryptWarningDialog)
{
    ui->setupUi(this);
    encryptWallet = false;
}

WalletNonEncryptWarningDialog::~WalletNonEncryptWarningDialog()
{
    delete ui;
}

void WalletNonEncryptWarningDialog::on_ignorePushButton_clicked()
{
    //encryptWallet = false;
    QDialog::done(false);
    //QDialog::close();
}

void WalletNonEncryptWarningDialog::on_encryptPushButton_clicked()
{
    //encryptWallet = true;
    QDialog::done(true);
}

void WalletNonEncryptWarningDialog::on_doNotShowAgainCheckBox_toggled(bool checked)
{

}
