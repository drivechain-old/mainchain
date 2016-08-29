#include "sidechaindepositdialog.h"
#include "ui_sidechaindepositdialog.h"

#include "base58.h"
#include "main.h"
#include "primitives/sidechain.h"
#include "primitives/transaction.h"
#include "txdb.h"

#include "wallet/wallet.h"

#include <QComboBox>

SidechainDepositDialog::SidechainDepositDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SidechainDepositDialog)
{
    ui->setupUi(this);

    // TODO it would be nice to show the user names instead of a hex...
    BOOST_FOREACH(uint256 id, validSidechains) {
        ui->comboBoxSidechains->addItem(QString::fromStdString(id.GetHex()));
    }
}

SidechainDepositDialog::~SidechainDepositDialog()
{
    delete ui;
}

void SidechainDepositDialog::on_pushButtonRequestAddress_clicked()
{
    // TODO request address from local sidechain node(s)
}

void SidechainDepositDialog::on_pushButtonDeposit_clicked()
{
    if (!psidechaintree)
        return;

    if (pwalletMain->IsLocked())
        return;

    // Lookup selected sidechain
    uint256 sidechainid = uint256S(ui->comboBoxSidechains->currentText().toStdString());
    sidechainSidechain sidechain;
    if (!psidechaintree->GetSidechain(sidechainid, sidechain))
        return;

    std::vector <CRecipient> vRecipient;

    // Payment to sidechain deposit script
    CRecipient payment = {sidechain.depositScript, ui->payAmount->value(), false};
    vRecipient.push_back(payment);

    // Create deposit transaction
    CReserveKey reserveKey(pwalletMain);
    CAmount nFee;
    int nChangePos = -1;
    CWalletTx wtx;
    std::string strError;
    if (!pwalletMain->CreateTransaction(vRecipient, wtx, reserveKey, nFee, nChangePos, strError))
    {
        // TODO make pretty
        QDialog error;
        error.setWindowTitle(QString::fromStdString(strError));
        error.exec();
        return;
    }

    if (!pwalletMain->CommitTransaction(wtx, reserveKey))
    {
        // TODO make pretty
        QDialog error;
        error.setWindowTitle("Failed to commit deposit transaction");
        error.exec();
        return;
    }

    // Deposit data (address, sidechainid)
    sidechainDeposit deposit;
    deposit.sidechainid = sidechain.GetHash();
    CBitcoinAddress depAddr = CBitcoinAddress(ui->lineEditAddress->text().toStdString());
    depAddr.GetKeyID(deposit.keyID);
    deposit.dt = wtx;

    // Calculate minimum ouput to be relayed
    CRecipient calc = {deposit.GetScript(), 0, false};
    CTxOut txoutCalc(calc.nAmount, calc.scriptPubKey);
    size_t nSize = txoutCalc.GetSerializeSize(SER_DISK, 0)+148u;
    CAmount minOutput = 3*::minRelayTxFee.GetFee(nSize);

    std::vector <CRecipient> vDataRecipient;
    CRecipient depositInfo = {deposit.GetScript(), minOutput, false};
    vDataRecipient.push_back(depositInfo);

    // Create data transaction
    CReserveKey dataReserveKey(pwalletMain);
    CAmount nDataFee;
    CWalletTx dtx;
    if (!pwalletMain->CreateTransaction(vDataRecipient, dtx, dataReserveKey, nDataFee, nChangePos, strError))
    {
        // TODO make pretty & inform the user that they
        // will need to resubmit the deposit
        QDialog error;
        error.setWindowTitle(QString::fromStdString(strError));
        error.exec();
        return;
    }

    if (!pwalletMain->CommitTransaction(dtx, dataReserveKey))
    {
        // TODO make pretty & inform the user that they
        // will need to resubmit the deposit
        QDialog error;
        error.setWindowTitle("Failed to commit deposit data transaction");
        error.exec();
        return;
    }
}
