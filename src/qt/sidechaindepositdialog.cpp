#include "sidechaindepositdialog.h"
#include "ui_sidechaindepositdialog.h"

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

    // Key to send the change to
    CReserveKey reserveKey(pwalletMain);

    // Fee to create the sidechain deposit transaction
    CAmount nFeeRequired;

    // Deposit TX outputs:
    vector<CRecipient> vecSend;

    // 0: Payment to sidechain deposit script
    CRecipient deposit = {sidechain.depositPubKey, ui->payAmount->value(), false};
    vecSend.push_back(deposit);

    // 1: Sidechain hash (OP_RETURN) & deposit address
    std::string addr = ui->lineEditAddress->text().toStdString();
    std::string hashHex = sidechain.GetHash().GetHex();

    CScript dataScript;
    dataScript << vector<unsigned char>(addr.begin(), addr.end());
    dataScript << OP_0; // Separator
    dataScript << vector<unsigned char>(hashHex.begin(), hashHex.end());
    dataScript << OP_RETURN;

    // Calculate the minimum output to be relayed by the network
    CRecipient dataCalc = {dataScript, 0, false};
    CTxOut txoutCalc(dataCalc.nAmount, dataCalc.scriptPubKey);
    size_t nSize = txoutCalc.GetSerializeSize(SER_DISK, 0)+148u;
    CAmount minOutput = 3*::minRelayTxFee.GetFee(nSize);

    CRecipient dataFinal = {dataScript, minOutput, false};
    vecSend.push_back(dataFinal);

    int nChangePos = -1;

    // Create a transaction with the above outputs
    CWalletTx wtx;
    std::string strError;
    if (!pwalletMain->CreateTransaction(vecSend, wtx, reserveKey, nFeeRequired, nChangePos, strError))
    {
        // TODO
        QDialog error;
        error.setWindowTitle(QString::fromStdString(strError));
        error.exec();
        return;
    }

    if (!pwalletMain->CommitTransaction(wtx, reserveKey))
        return;
}
