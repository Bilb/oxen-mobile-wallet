#include <stdint.h>
#include "cstdlib"
#include <chrono>
#include <functional>
#include <iostream>
#include <unistd.h>
#include "thread"
#include "../External/android/monero/include/wallet2_api.h"

using namespace std::chrono_literals;

namespace Oxen = Wallet;

#ifdef __cplusplus
extern "C"
{
#endif

struct Utf8Box
{
    char *value;

    Utf8Box(char *_value)
    {
        value = _value;
    }
};

struct SubaddressRow
{
    uint64_t id;
    char *address;
    char *label;

    SubaddressRow(std::size_t _id, char *_address, char *_label)
    {
        id = static_cast<uint64_t>(_id);
        address = _address;
        label = _label;
    }
};

struct AccountRow
{
    uint64_t id;
    char *label;

    AccountRow(std::size_t _id, char *_label)
    {
        id = static_cast<uint64_t>(_id);
        label = _label;
    }
};

struct TransactionInfoRow
{
    uint64_t amount;
    uint64_t fee;
    uint64_t blockHeight;
    uint64_t confirmations;
    uint32_t subaddrAccount;
    int8_t direction;
    int8_t isPending;
    
    char *hash;
    char *paymentId;

    int64_t datetime;

    TransactionInfoRow(Oxen::TransactionInfo *transaction)
    {
        amount = transaction->amount();
        fee = transaction->fee();
        blockHeight = transaction->blockHeight();
        subaddrAccount = transaction->subaddrAccount();
        confirmations = transaction->confirmations();
        datetime = static_cast<int64_t>(transaction->timestamp());            
        direction = transaction->direction();
        isPending = static_cast<int8_t>(transaction->isPending());
        std::string *hash_str = new std::string(transaction->hash());
        hash = strdup(hash_str->c_str());
        paymentId = strdup(transaction->paymentId().c_str());
    }
};

struct PendingTransactionRaw
{
    uint64_t amount;
    uint64_t fee;
    char *hash;
    Oxen::PendingTransaction *transaction;

    PendingTransactionRaw(Oxen::PendingTransaction *_transaction)
    {
        transaction = _transaction;
        amount = _transaction->amount();
        fee = _transaction->fee();
        hash = strdup(_transaction->txid()[0].c_str());
    }
};

Oxen::Wallet *m_wallet;
Oxen::TransactionHistory *m_transaction_history;
Oxen::Subaddress *m_subaddress;
Oxen::SubaddressAccount *m_account;
uint64_t m_last_known_wallet_height;
std::mutex m_store_mtx;

void change_current_wallet(Oxen::Wallet *wallet)
{
    m_wallet = wallet;
     m_transaction_history = nullptr;
     m_account = nullptr;
     m_subaddress = nullptr;
    

    if (wallet != nullptr)
    {
        m_transaction_history = wallet->history();
    }

    if (wallet != nullptr)
    {
        m_account = wallet->subaddressAccount();
    }

    if (wallet != nullptr)
    {
        m_subaddress = wallet->subaddress();
    }
}

Oxen::Wallet *get_current_wallet()
{
    return m_wallet;
}

bool create_wallet(char *path, char *password, char *language, int32_t networkType, char *error)
{
    Oxen::NetworkType _networkType = static_cast<Oxen::NetworkType>(networkType);
    Oxen::WalletManagerBase *walletManager = Oxen::WalletManagerFactory::getWalletManager();
    Oxen::Wallet *wallet = walletManager->createWallet(path, password, language, _networkType);

    // int status;
    // std::string errorString;

    auto stat = wallet->status();

    auto& [status, errorString] = stat;

    if (status != Oxen::Wallet::Status_Ok)
    {
        error = strdup(errorString.c_str());
        return false;
    }

    walletManager->closeWallet(wallet);
    wallet = walletManager->openWallet(std::string(path), std::string(password), _networkType);
    
    stat = wallet->status();

    if (status != Oxen::Wallet::Status_Ok)
    {
        error = strdup(errorString.c_str());
        return false;
    }

    change_current_wallet(wallet);

    return true;
}

bool restore_wallet_from_seed(char *path, char *password, char *seed, int32_t networkType, uint64_t restoreHeight, char *error)
{
    Oxen::NetworkType _networkType = static_cast<Oxen::NetworkType>(networkType);
    Oxen::Wallet *wallet = Oxen::WalletManagerFactory::getWalletManager()->recoveryWallet(
        std::string(path),
        std::string(password),
        std::string(seed),
        _networkType,
        (uint64_t)restoreHeight);

    // int status;
    // std::string errorString;

    auto [status, errorString] = wallet->status();

    if (status != Oxen::Wallet::Status_Ok)
    {
        error = strdup(errorString.c_str());
        return false;
    }

    change_current_wallet(wallet);
    return true;
}

bool restore_wallet_from_keys(char *path, char *password, char *language, char *address, char *viewKey, char *spendKey, int32_t networkType, uint64_t restoreHeight, char *error)
{
    Oxen::NetworkType _networkType = static_cast<Oxen::NetworkType>(networkType);
    Oxen::Wallet *wallet = Oxen::WalletManagerFactory::getWalletManager()->createWalletFromKeys(
        std::string(path),
        std::string(password),
        std::string(language),
        _networkType,
        (uint64_t)restoreHeight,
        std::string(address),
        std::string(viewKey),
        std::string(spendKey));

    // int status;
    // std::string errorString;

    auto [status, errorString] = wallet->status();

    if (status != Oxen::Wallet::Status_Ok)
    {
        error = strdup(errorString.c_str());
        return false;
    }

    change_current_wallet(wallet);
    return true;
}

void load_wallet(char *path, char *password, int32_t nettype)
{
    Oxen::NetworkType networkType = static_cast<Oxen::NetworkType>(nettype);
    Oxen::WalletManagerBase *walletManager = Oxen::WalletManagerFactory::getWalletManager();
    Oxen::Wallet *wallet = walletManager->openWallet(std::string(path), std::string(password), networkType);
    change_current_wallet(wallet);
}

bool is_wallet_exist(char *path)
{
    return Oxen::WalletManagerFactory::getWalletManager()->walletExists(std::string(path));
}

void close_current_wallet()
{
    Oxen::WalletManagerFactory::getWalletManager()->closeWallet(get_current_wallet());
    change_current_wallet(nullptr);
}

char *get_filename()
{
    return strdup(get_current_wallet()->filename().c_str());
}

char *secret_view_key()
{
    return strdup(get_current_wallet()->secretViewKey().c_str());
}

char *public_view_key()
{
    return strdup(get_current_wallet()->publicViewKey().c_str());
}

char *secret_spend_key()
{
    return strdup(get_current_wallet()->secretSpendKey().c_str());
}

char *public_spend_key()
{
    return strdup(get_current_wallet()->publicSpendKey().c_str());
}

char *get_address(uint32_t account_index, uint32_t address_index)
{
    return strdup(get_current_wallet()->address(account_index, address_index).c_str());
}


const char *seed()
{
    return strdup(get_current_wallet()->seed().c_str());
}

uint64_t get_full_balance(uint32_t account_index)
{
    return get_current_wallet()->balance(account_index);
}

uint64_t get_unlocked_balance(uint32_t account_index)
{
    return get_current_wallet()->unlockedBalance(account_index);
}

uint64_t get_current_height()
{
    return get_current_wallet()->blockChainHeight();
}

uint64_t get_node_height()
{
    return get_current_wallet()->daemonBlockChainHeight();
}

bool connect_to_node(char *error)
{
    bool is_connected = get_current_wallet()->connectToDaemon();

    if (!is_connected)
    {
        error = strdup(get_current_wallet()->status().second.c_str());
    }

    return is_connected;
}

bool setup_node(char *address, char *login, char *password, bool use_ssl, bool is_light_wallet, char *error)
{
    nice(19);
    Oxen::Wallet *wallet = get_current_wallet();
    
    std::string _login = "";
    std::string _password = "";

    if (login != nullptr)
    {
        _login = std::string(login);
    }

    if (password != nullptr)
    {
        _password = std::string(password);
    }

    bool inited = wallet->init(std::string(address), 0, _login, _password, use_ssl, is_light_wallet);

    if (!inited)
    {
        error = strdup(wallet->status().second.c_str());
    } else if (!wallet->connectToDaemon()) {
        inited = false;
        error = strdup(wallet->status().second.c_str());
    }

    return inited;
}

bool is_connected()
{
    return get_current_wallet()->connected();
}

void start_refresh()
{
    get_current_wallet()->refreshAsync();
    get_current_wallet()->startRefresh();
}

void set_refresh_from_block_height(uint64_t height)
{
    get_current_wallet()->setRefreshFromBlockHeight(height);
}

void set_recovering_from_seed(bool is_recovery)
{
    get_current_wallet()->setRecoveringFromSeed(is_recovery);
}

void store()
{
    m_store_mtx.lock();
    get_current_wallet()->store("");
    m_store_mtx.unlock();
}

bool transaction_create(char *address, char *payment_id, char *amount,
                                          uint8_t priority, uint32_t subaddr_account, Utf8Box &error, PendingTransactionRaw &pendingTransaction)
{
    nice(19);
    
    //auto priority = static_cast<Oxen::PendingTransaction::Priority>(priority_raw);
    std::string _payment_id;
    Oxen::PendingTransaction *transaction;

    if (payment_id != nullptr)
    {
        _payment_id = std::string(payment_id);
    }

    if (amount != nullptr)
    {
        uint64_t _amount = Oxen::Wallet::amountFromString(std::string(amount));
        transaction = m_wallet->createTransaction(std::string(address), _amount, priority, subaddr_account);
    }
    else
    {
        transaction = m_wallet->createTransaction(std::string(address), std::optional<uint64_t>(), priority, subaddr_account);
    }
    
    int status = transaction->status().first;

    if (status == Oxen::PendingTransaction::Status::Status_Error || status == Oxen::PendingTransaction::Status::Status_Critical)
    {
        error = Utf8Box(strdup(transaction->status().second.c_str()));
        return false;
    }

    pendingTransaction = PendingTransactionRaw(transaction);
    return true;
}

bool transaction_commit(PendingTransactionRaw *transaction, Utf8Box &error)
{
    bool committed = transaction->transaction->commit();

    if (!committed)
    {
        error = Utf8Box(strdup(transaction->transaction->status().second.c_str()));
    }

    return committed;
}

int64_t *subaddrress_get_all()
{
    std::vector<Oxen::SubaddressRow *> _subaddresses = m_subaddress->getAll();
    size_t size = _subaddresses.size();
    int64_t *subaddresses = (int64_t *)malloc(size * sizeof(int64_t));

    for (int i = 0; i < size; i++)
    {
        Oxen::SubaddressRow *row = _subaddresses[i];
        SubaddressRow *_row = new SubaddressRow(row->getRowId(), strdup(row->getAddress().c_str()), strdup(row->getLabel().c_str()));
        subaddresses[i] = reinterpret_cast<int64_t>(_row);
    }

    return subaddresses;
}

int32_t subaddrress_size()
{
    std::vector<Oxen::SubaddressRow *> _subaddresses = m_subaddress->getAll();
    return _subaddresses.size();
}

void subaddress_add_row(uint32_t accountIndex, char *label)
{
    m_subaddress->addRow(accountIndex, std::string(label));
}

void subaddress_set_label(uint32_t accountIndex, uint32_t addressIndex, char *label)
{
    m_subaddress->setLabel(accountIndex, addressIndex, std::string(label));
}

void subaddress_refresh(uint32_t accountIndex)
{
    m_subaddress->refresh(accountIndex);
}

int32_t account_size()
{
    std::vector<Oxen::SubaddressAccountRow *> _accocunts = m_account->getAll();
    return _accocunts.size();
}

int64_t *account_get_all()
{
    std::vector<Oxen::SubaddressAccountRow *> _accocunts = m_account->getAll();
    size_t size = _accocunts.size();
    int64_t *accocunts = (int64_t *)malloc(size * sizeof(int64_t));

    for (int i = 0; i < size; i++)
    {
        Oxen::SubaddressAccountRow *row = _accocunts[i];
        AccountRow *_row = new AccountRow(row->getRowId(), strdup(row->getLabel().c_str()));
        accocunts[i] = reinterpret_cast<int64_t>(_row);
    }

    return accocunts;
}

void account_add_row(char *label)
{
    m_account->addRow(std::string(label));
}

void account_set_label_row(uint32_t account_index, char *label)
{
    m_account->setLabel(account_index, label);
}

void account_refresh()
{
    m_account->refresh();
}

int64_t *transactions_get_all()
{
    std::vector<Oxen::TransactionInfo *> transactions = m_transaction_history->getAll();
    size_t size = transactions.size();
    int64_t *transactionAddresses = (int64_t *)malloc(size * sizeof(int64_t));

    for (int i = 0; i < size; i++)
    {
        Oxen::TransactionInfo *row = transactions[i];
        TransactionInfoRow *tx = new TransactionInfoRow(row);
        transactionAddresses[i] = reinterpret_cast<int64_t>(tx);
    }

    return transactionAddresses;
}

void transactions_refresh()
{
    m_transaction_history->refresh();
}

int64_t transactions_count()
{
    return m_transaction_history->count();
}

int LedgerExchange(
    unsigned char *command,
    unsigned int cmd_len,
    unsigned char *response,
    unsigned int max_resp_len)
{
    return -1;
}

int LedgerFind(char *buffer, size_t len)
{
    return -1;
}

void on_startup()
{
    Oxen::Utils::onStartup();
}

void rescan_blockchain()
{
    m_wallet->rescanBlockchainAsync();
}

#ifdef __cplusplus
}
#endif
