#include "xbridgewalletconnectoreth.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#include "base58.h"
#include "uint256.h"

#include "util/logger.h"
#include "util/txlog.h"

#include "xkey.h"
#include "xbitcoinsecret.h"
#include "xbitcoinaddress.h"
#include "xbitcointransaction.h"

#include <boost/asio.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ssl.hpp>
#include <stdio.h>

namespace xbridge
{

namespace rpc
{

namespace
{

using namespace json_spirit;
using namespace std;
using namespace boost;
using namespace boost::asio;

Object CallRPC(const std::string & rpcuser, const std::string & rpcpasswd,
               const std::string & rpcip, const std::string & rpcport,
               const std::string & strMethod, const Array & params);

//*****************************************************************************
//*****************************************************************************
bool getAccounts(const std::string & rpcuser,
                 const std::string & rpcpasswd,
                 const std::string & rpcip,
                 const std::string & rpcport,
                 std::vector<std::string> & accounts)
{
    try
    {
        LOG() << "rpc call <eth_accounts>";

        Array params;
        Object reply = CallRPC(rpcuser, rpcpasswd, rpcip, rpcport,
                               "eth_accounts", params);

        // Parse reply
        const Value & result = find_value(reply, "result");
        const Value & error  = find_value(reply, "error");

        if (error.type() != null_type)
        {
            // Error
            LOG() << "error: " << write_string(error, false);
            return false;
        }
        else if (result.type() != array_type)
        {
            // Result
            LOG() << "result not an array " <<
                     (result.type() == null_type ? "" :
                      result.type() == str_type  ? result.get_str() :
                                                   write_string(result, true));
            return false;
        }

        Array arr = result.get_array();
        for (const Value & v : arr)
        {
            if (v.type() == str_type)
            {
                accounts.push_back(v.get_str());
            }
        }
    }
    catch (std::exception & e)
    {
        LOG() << "getAccounts exception " << e.what();
        return false;
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
bool sendRawTransaction(const std::string & rpcuser,
                        const std::string & rpcpasswd,
                        const std::string & rpcip,
                        const std::string & rpcport,
                        const std::string & rawtx,
                        string & txid,
                        int32_t & errorCode)
{
    try
    {
        LOG() << "rpc call <eth_sendRawTransaction>";

        errorCode = 0;

        Array params;
        params.push_back(rawtx);
        Object reply = CallRPC(rpcuser, rpcpasswd, rpcip, rpcport,
                               "eth_sendRawTransaction", params);

        // Parse reply
        const Value & error  = find_value(reply, "error");
        if (error.type() != null_type)
        {
            // Error
            LOG() << "error: " << write_string(error, false);
            errorCode = find_value(error.get_obj(), "code").get_int();
            return false;
        }

        const Value & result = find_value(reply, "result");
        if (result.type() != str_type)
        {
            // Result
            LOG() << "result not an string " << write_string(result, true);
            return false;
        }

        txid = result.get_str();
    }
    catch (std::exception & e)
    {
        errorCode = -1;

        LOG() << "sendRawTransaction exception " << e.what();
        return false;
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
bool getTransactionByHash(const std::string & rpcuser,
                          const std::string & rpcpasswd,
                          const std::string & rpcip,
                          const std::string & rpcport,
                          const std::string & txHash,
                          uint256 & txBlockNumber)
{
    try
    {
        LOG() << "rpc call <eth_getTransactionByHash>";

        Array params;
        params.push_back(txHash);
        Object reply = CallRPC(rpcuser, rpcpasswd, rpcip, rpcport,
                               "eth_getTransactionByHash", params);

        // Parse reply
        const Value & error  = find_value(reply, "error");
        if (error.type() != null_type)
        {
            // Error
            LOG() << "error: " << write_string(error, false);
            int errorCode = find_value(error.get_obj(), "code").get_int();
            return false;
        }

        const Value & result = find_value(reply, "result");
        if (result.type() != obj_type)
        {
            // Result
            LOG() << "result not an string " << write_string(result, true);
            return false;
        }

        const Value & blockNumber = find_value(result.get_obj(), "blockNumber");
        if(blockNumber.type() != str_type)
        {
            LOG() << "blockNumber not an string " << write_string(blockNumber, true);
            return false;
        }

        txBlockNumber = uint256(blockNumber.get_str());
    }
    catch (std::exception & e)
    {
        LOG() << "getTransactionByHash exception " << e.what();
        return false;
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
bool getBlockNumber(const std::string & rpcuser,
                    const std::string & rpcpasswd,
                    const std::string & rpcip,
                    const std::string & rpcport,
                    uint256 & blockNumber)
{
    try
    {
        LOG() << "rpc call <eth_blockNumber>";

        Array params;
        Object reply = CallRPC(rpcuser, rpcpasswd, rpcip, rpcport,
                               "eth_blockNumber", params);

        // Parse reply
        const Value & result = find_value(reply, "result");
        const Value & error  = find_value(reply, "error");

        if (error.type() != null_type)
        {
            // Error
            LOG() << "error: " << write_string(error, false);
            return false;
        }
        else if (result.type() != str_type)
        {
            // Result
            LOG() << "result not an string " << write_string(result, true);
            return false;
        }

        blockNumber = uint256(result.get_str());
    }
    catch (std::exception & e)
    {
        LOG() << "getBlockNumber exception " << e.what();
        return false;
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
bool getGasPrice(const std::string & rpcuser,
                 const std::string & rpcpasswd,
                 const std::string & rpcip,
                 const std::string & rpcport,
                 uint256 & gasPrice)
{
    try
    {
        LOG() << "rpc call <eth_gasPrice>";

        Array params;
        Object reply = CallRPC(rpcuser, rpcpasswd, rpcip, rpcport,
                               "eth_gasPrice", params);

        // Parse reply
        const Value & result = find_value(reply, "result");
        const Value & error  = find_value(reply, "error");

        if (error.type() != null_type)
        {
            // Error
            LOG() << "error: " << write_string(error, false);
            return false;
        }
        else if (result.type() != str_type)
        {
            // Result
            LOG() << "result not a string ";
            return false;
        }

        gasPrice = uint256(result.get_str());
    }
    catch (std::exception & e)
    {
        LOG() << "getGasPrice exception " << e.what();
        return false;
    }

    return true;
}

//*****************************************************************************
//*****************************************************************************
bool getEstimateGas(const std::string & rpcuser,
                    const std::string & rpcpasswd,
                    const std::string & rpcip,
                    const std::string & rpcport,
                    const uint160 & from,
                    const uint160 & to,
                    const uint256 & gasPrice,
                    const bytes & data,
                    uint256 & estimateGas)
{
    try
    {
        LOG() << "rpc call <eth_estimateGas>";

        Array params;

        Object transaction;
        transaction.push_back(Pair("from", from.ToString()));
        transaction.push_back(Pair("to", to.ToString()));
        transaction.push_back(Pair("gasPrice", gasPrice.ToString()));
        transaction.push_back(Pair("data", asString(data)));

        params.push_back(transaction);
        params.push_back("latest");

        Object reply = CallRPC(rpcuser, rpcpasswd, rpcip, rpcport,
                               "eth_estimateGas", params);

        // Parse reply
        const Value & result = find_value(reply, "result");
        const Value & error  = find_value(reply, "error");

        if (error.type() != null_type)
        {
            // Error
            LOG() << "error: " << write_string(error, false);
            return false;
        }
        else if (result.type() != str_type)
        {
            // Result
            LOG() << "result not a string ";
            return false;
        }

        estimateGas = uint256(result.get_str());
    }
    catch (std::exception & e)
    {
        LOG() << "getEstimateGas exception " << e.what();
        return false;
    }

    return true;
}

} // namespace

} // namespace rpc

EthWalletConnector::EthWalletConnector()
{

}

//*****************************************************************************
//*****************************************************************************
std::string EthWalletConnector::fromXAddr(const std::vector<unsigned char> & xaddr) const
{
    return std::string(xaddr.begin() + 2, xaddr.end());
}

//*****************************************************************************
//*****************************************************************************
std::vector<unsigned char> EthWalletConnector::toXAddr(const std::string & addr) const
{
    std::vector<unsigned char> vch = addrPrefix[0];
    vch.insert(vch.end(), addr.begin(), addr.end());

    return vch;
}

//*****************************************************************************
//*****************************************************************************
bool EthWalletConnector::requestAddressBook(std::vector<wallet::AddressBookEntry> & entries)
{
    std::vector<std::string> accounts;
    if (!rpc::getAccounts(m_user, m_passwd, m_ip, m_port, accounts))
        return false;

    entries.push_back(std::make_pair("default", accounts));

    return true;
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::getUnspent(std::vector<wallet::UtxoEntry> & inputs) const
{
    return true;
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::getNewAddress(std::string & /*addr*/)
{
    return true;
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::sendRawTransaction(const std::string & rawtx,
                                            std::string & txid,
                                            int32_t & errorCode)
{
    if (!rpc::sendRawTransaction(m_user, m_passwd, m_ip, m_port,
                                    rawtx, txid, errorCode))
    {
        LOG() << "rpc::sendRawTransaction failed" << __FUNCTION__;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::newKeyPair(std::vector<unsigned char> & pubkey,
                                    std::vector<unsigned char> & privkey)
{
    xbridge::CKey km;
    km.MakeNewKey(true);

    xbridge::CPubKey pub = km.GetPubKey();
    pubkey = std::vector<unsigned char>(pub.begin(), pub.end());
    privkey = std::vector<unsigned char>(km.begin(), km.end());

    return true;
}

//******************************************************************************
//******************************************************************************
std::vector<unsigned char> EthWalletConnector::getKeyId(const std::vector<unsigned char> & /*pubkey*/)
{
    return std::vector<unsigned char>();
}

//******************************************************************************
//******************************************************************************
std::vector<unsigned char> EthWalletConnector::getScriptId(const std::vector<unsigned char> & /*script*/)
{
    return std::vector<unsigned char>();
}

//******************************************************************************
//******************************************************************************
std::string EthWalletConnector::scriptIdToString(const std::vector<unsigned char> & /*id*/) const
{
    return std::string();
}

//******************************************************************************
// calculate tx fee for deposit tx
// output count always 1
//******************************************************************************
double EthWalletConnector::minTxFee1(const uint32_t inputCount, const uint32_t outputCount)
{
    uint64_t fee = (148*inputCount + 34*outputCount + 10) * feePerByte;
    if (fee < minTxFee)
    {
        fee = minTxFee;
    }
    return (double)fee / COIN;
}

//******************************************************************************
// calculate tx fee for payment/refund tx
// input count always 1
//******************************************************************************
double EthWalletConnector::minTxFee2(const uint32_t inputCount, const uint32_t outputCount)
{
    uint64_t fee = (180*inputCount + 34*outputCount + 10) * feePerByte;
    if (fee < minTxFee)
    {
        fee = minTxFee;
    }
    return (double)fee / COIN;
}

//******************************************************************************
// return false if deposit tx not found (need wait tx)
// true if tx found and checked
// isGood == true id depost tx is OK
//******************************************************************************
bool EthWalletConnector::checkTransaction(const std::string & depositTxId,
                                          const std::string & /*destination*/,
                                          const uint64_t & /*amount*/,
                                          bool & isGood)
{
    isGood  = false;

    uint256 txBlockNumber;
    if (!rpc::getTransactionByHash(m_user, m_passwd, m_ip, m_port, depositTxId, txBlockNumber))
    {
        LOG() << "no tx found " << depositTxId << " " << __FUNCTION__;
        return false;
    }

    uint256 lastBlockNumber;
    if (!rpc::getBlockNumber(m_user, m_passwd, m_ip, m_port, lastBlockNumber))
    {
        LOG() << "can't get last block number " << depositTxId << " " << __FUNCTION__;
        return false;
    }

    if (requiredConfirmations > 0 && requiredConfirmations > (lastBlockNumber - txBlockNumber))
    {
        LOG() << "tx " << depositTxId << " unconfirmed, need " << requiredConfirmations << " " << __FUNCTION__;
        return false;
    }

    // TODO check amount in tx

    isGood = true;

    return true;
}

//******************************************************************************
//******************************************************************************
uint32_t EthWalletConnector::lockTime(const char role) const
{
    uint256 lastBlockNumber;
    if (!rpc::getBlockNumber(m_user, m_passwd, m_ip, m_port, lastBlockNumber))
    {
        LOG() << "blockchain info not received " << __FUNCTION__;
        return 0;
    }

    if (lastBlockNumber == 0)
    {
        LOG() << "block count not defined in blockchain info " << __FUNCTION__;
        return 0;
    }

    // lock time
    uint256 lt = 0;
    if (role == 'A')
    {
        // 2h in seconds
        lt = lastBlockNumber + 120 / blockTime;
    }
    else if (role == 'B')
    {
        // 1h in seconds
        lt = lastBlockNumber + 36 / blockTime;
    }

    return lt.GetCompact();
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::createDepositUnlockScript(const std::vector<unsigned char> & /*myPubKey*/,
                                                   const std::vector<unsigned char> & /*otherPubKey*/,
                                                   const std::vector<unsigned char> & /*xdata*/,
                                                   const uint32_t /*lockTime*/,
                                                   std::vector<unsigned char> & resultSript)
{
    resultSript = std::vector<unsigned char>();
    return true;
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::createDepositTransaction(const std::vector<std::pair<std::string, int> > & /*inputs*/,
                                                  const std::vector<std::pair<std::string, double> > & /*outputs*/,
                                                  std::string & txId,
                                                  std::string & rawTx)
{
    bytes initiateMethodSignature = EthEncoder::encodeSig("initiate(bytes20,address,uint256)");
    bytes respondMethodSignature = EthEncoder::encodeSig("respond(bytes20,address,uint256)");

    EthTransaction tr;

    uint256 gasPrice;
    if(!rpc::getGasPrice(m_user, m_passwd, m_ip, m_port, gasPrice))
    {
        LOG() << "can't get gasPrice" << __FUNCTION__;
        return false;
    }


    tr.gasPrice = gasPrice.ToString();
    tr.to = contractAddress;

    return true;
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::createRefundTransaction(const std::vector<std::pair<std::string, int> > & /*inputs*/,
                                                 const std::vector<std::pair<std::string, double> > & /*outputs*/,
                                                 const std::vector<unsigned char> & /*mpubKey*/,
                                                 const std::vector<unsigned char> & /*mprivKey*/,
                                                 const std::vector<unsigned char> & /*innerScript*/,
                                                 const uint32_t /*lockTime*/,
                                                 std::string & txId,
                                                 std::string & rawTx)
{
    bytes refundMethodSignature = EthEncoder::encodeSig("refund(bytes20)");

    return true;
}

//******************************************************************************
//******************************************************************************
bool EthWalletConnector::createPaymentTransaction(const std::vector<std::pair<std::string, int> > & /*inputs*/,
                                                  const std::vector<std::pair<std::string, double> > & /*outputs*/,
                                                  const std::vector<unsigned char> & /*mpubKey*/,
                                                  const std::vector<unsigned char> & /*mprivKey*/,
                                                  const std::vector<unsigned char> & /*xpubKey*/,
                                                  const std::vector<unsigned char> & /*innerScript*/,
                                                  std::string & txId,
                                                  std::string & rawTx)
{
    bytes redeemMethodSignature = EthEncoder::encodeSig("redeem(bytes20,bytes)");

    return true;
}

string EthWalletConnector::signTransaction(const EthTransaction& transaction)
{

}

} //namespace xbridge