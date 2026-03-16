/**
 * BankSystem.cpp
 *
 * Implementation of Account, SavingsAccount, CheckingAccount, and Bank.
 * ~300 lines.  No main() — designed as a reusable library component.
 */

#include "BankSystem.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cmath>

// ─── File-scope helpers ───────────────────────────────────────────────────────

static std::string currentTimestamp() {
    std::time_t t = std::time(nullptr);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

static std::string typeName(Transaction::Type t) {
    switch (t) {
        case Transaction::Type::DEPOSIT:      return "DEPOSIT";
        case Transaction::Type::WITHDRAWAL:   return "WITHDRAWAL";
        case Transaction::Type::TRANSFER_IN:  return "TRANSFER_IN";
        case Transaction::Type::TRANSFER_OUT: return "TRANSFER_OUT";
        case Transaction::Type::INTEREST:     return "INTEREST";
    }
    return "UNKNOWN";
}

// ════════════════════════════════════════════════════════════════════════════
// Account
// ════════════════════════════════════════════════════════════════════════════

Account::Account(int id, const std::string& owner, double initial_balance)
    : account_id(id), owner_name(owner), balance(0.0), is_active(true)
{
    if (id <= 0)
        throw std::invalid_argument("Account ID must be positive.");
    if (owner.empty())
        throw std::invalid_argument("Owner name must not be empty.");
    if (initial_balance < 0.0)
        throw std::invalid_argument("Initial balance cannot be negative.");

    if (initial_balance > 0.0) {
        balance = initial_balance;
        history.push_back({Transaction::Type::DEPOSIT, initial_balance,
                           balance, currentTimestamp(), "Initial deposit"});
    }
}

Account::~Account() {}

// ── Getters ──────────────────────────────────────────────────────────────────
int         Account::getID()           const { return account_id; }
std::string Account::getOwnerName()    const { return owner_name; }
double      Account::getBalance()      const { return balance; }
bool        Account::isActive()        const { return is_active; }
int         Account::getTransactionCount() const {
    return static_cast<int>(history.size());
}
const std::vector<Transaction>& Account::getTransactionHistory() const {
    return history;
}

// ── Setter ───────────────────────────────────────────────────────────────────
void Account::setOwnerName(const std::string& new_name) {
    if (new_name.empty())
        throw std::invalid_argument("Owner name must not be empty.");
    owner_name = new_name;
}

// ── deposit ──────────────────────────────────────────────────────────────────
void Account::deposit(double amount) {
    if (!is_active)
        throw std::runtime_error("Cannot operate on a closed account.");
    if (amount <= 0.0)
        throw std::invalid_argument("Deposit amount must be positive.");
    balance += amount;
    history.push_back({Transaction::Type::DEPOSIT, amount,
                       balance, currentTimestamp(), "Deposit"});
}

// ── withdraw (base — no overdraft, no minimum) ───────────────────────────────
bool Account::withdraw(double amount) {
    if (!is_active)
        throw std::runtime_error("Cannot operate on a closed account.");
    if (amount <= 0.0)
        throw std::invalid_argument("Withdrawal amount must be positive.");
    if (amount > balance) return false;
    balance -= amount;
    history.push_back({Transaction::Type::WITHDRAWAL, amount,
                       balance, currentTimestamp(), "Withdrawal"});
    return true;
}

// ── transfer ─────────────────────────────────────────────────────────────────
bool Account::transfer(double amount, Account& target) {
    if (!is_active || !target.is_active)
        throw std::runtime_error("Cannot transfer involving a closed account.");
    if (&target == this)
        throw std::invalid_argument("Cannot transfer to the same account.");
    if (amount <= 0.0)
        throw std::invalid_argument("Transfer amount must be positive.");

    if (!withdraw(amount)) return false;           // virtual — respects subclass rules

    // Relabel the withdrawal record as an outgoing transfer
    history.back().type        = Transaction::Type::TRANSFER_OUT;
    history.back().description = "Transfer out to account #"
                                 + std::to_string(target.account_id);

    // Credit the target directly (bypass target's virtual withdraw)
    target.balance += amount;
    target.history.push_back({Transaction::Type::TRANSFER_IN, amount,
                               target.balance, currentTimestamp(),
                               "Transfer in from account #"
                               + std::to_string(account_id)});
    return true;
}

// ── close / reactivate ───────────────────────────────────────────────────────
void Account::close() {
    if (!is_active)
        throw std::runtime_error("Account is already closed.");
    is_active = false;
}

void Account::reactivate() {
    if (is_active)
        throw std::runtime_error("Account is already active.");
    is_active = true;
}

// ── Aggregate helpers ─────────────────────────────────────────────────────────
double Account::getTotalDeposited() const {
    double total = 0.0;
    for (const auto& tx : history) {
        if (tx.type == Transaction::Type::DEPOSIT    ||
            tx.type == Transaction::Type::TRANSFER_IN ||
            tx.type == Transaction::Type::INTEREST)
            total += tx.amount;
    }
    return total;
}

double Account::getTotalWithdrawn() const {
    double total = 0.0;
    for (const auto& tx : history) {
        if (tx.type == Transaction::Type::WITHDRAWAL  ||
            tx.type == Transaction::Type::TRANSFER_OUT)
            total += tx.amount;
    }
    return total;
}

// ── Statement ─────────────────────────────────────────────────────────────────
std::string Account::getStatementSummary() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "=== Statement: #" << account_id << " [" << owner_name << "] ===\n";
    for (const auto& tx : history) {
        oss << "[" << tx.timestamp << "] "
            << std::left  << std::setw(13) << typeName(tx.type)
            << "  " << std::right << std::setw(9) << tx.amount
            << "  bal=" << tx.balance_after
            << "  " << tx.description << "\n";
    }
    oss << "Current balance: " << balance << "\n";
    return oss.str();
}

// ── getInfo (base) ────────────────────────────────────────────────────────────
std::string Account::getInfo() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Account ID : " << account_id                   << "\n"
        << "Owner      : " << owner_name                   << "\n"
        << "Type       : " << getType()                    << "\n"
        << "Balance    : " << balance                      << "\n"
        << "Tx Count   : " << history.size()               << "\n"
        << "Status     : " << (is_active ? "Active" : "Closed");
    return oss.str();
}

// ════════════════════════════════════════════════════════════════════════════
// SavingsAccount
// ════════════════════════════════════════════════════════════════════════════

SavingsAccount::SavingsAccount(int id, const std::string& owner,
                               double initial_balance,
                               double interest_rate_val,
                               double min_balance)
    : Account(id, owner, initial_balance),
      interest_rate(interest_rate_val),
      minimum_balance(min_balance)
{
    if (interest_rate_val < 0.0)
        throw std::invalid_argument("Interest rate cannot be negative.");
    if (min_balance < 0.0)
        throw std::invalid_argument("Minimum balance cannot be negative.");
    if (initial_balance > 0.0 && initial_balance < min_balance)
        throw std::invalid_argument(
            "Initial balance is below the minimum balance requirement.");
}

double SavingsAccount::getInterestRate()   const { return interest_rate; }
double SavingsAccount::getMinimumBalance() const { return minimum_balance; }

void SavingsAccount::setInterestRate(double rate) {
    if (rate < 0.0)
        throw std::invalid_argument("Interest rate cannot be negative.");
    interest_rate = rate;
}

// ── applyInterest: credits compound interest, returns amount credited ─────────
double SavingsAccount::applyInterest() {
    if (!isActive())
        throw std::runtime_error("Cannot apply interest on a closed account.");
    double interest = std::round(balance * interest_rate * 100.0) / 100.0;
    if (interest <= 0.0) return 0.0;
    balance += interest;
    history.push_back({Transaction::Type::INTEREST, interest, balance,
                       currentTimestamp(),
                       "Interest at "
                       + std::to_string(static_cast<int>(interest_rate * 100))
                       + "% pa"});
    return interest;
}

// ── withdraw: enforces minimum_balance ───────────────────────────────────────
bool SavingsAccount::withdraw(double amount) {
    if (!isActive())
        throw std::runtime_error("Cannot operate on a closed account.");
    if (amount <= 0.0)
        throw std::invalid_argument("Withdrawal amount must be positive.");
    if (balance - amount < minimum_balance) return false;  // would breach minimum
    balance -= amount;
    history.push_back({Transaction::Type::WITHDRAWAL, amount,
                       balance, currentTimestamp(), "Withdrawal"});
    return true;
}

std::string SavingsAccount::getType() const { return "Savings"; }

std::string SavingsAccount::getInfo() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << Account::getInfo()                                 << "\n"
        << "Interest   : " << (interest_rate * 100.0) << "% pa\n"
        << "Min Balance: " << minimum_balance;
    return oss.str();
}

// ════════════════════════════════════════════════════════════════════════════
// CheckingAccount
// ════════════════════════════════════════════════════════════════════════════

CheckingAccount::CheckingAccount(int id, const std::string& owner,
                                 double initial_balance,
                                 double overdraft_limit_val,
                                 double overdraft_fee_val)
    : Account(id, owner, initial_balance),
      overdraft_limit(overdraft_limit_val),
      overdraft_fee(overdraft_fee_val),
      overdraft_count(0)
{
    if (overdraft_limit_val < 0.0)
        throw std::invalid_argument("Overdraft limit cannot be negative.");
    if (overdraft_fee_val < 0.0)
        throw std::invalid_argument("Overdraft fee cannot be negative.");
}

double CheckingAccount::getOverdraftLimit() const { return overdraft_limit; }
double CheckingAccount::getOverdraftFee()   const { return overdraft_fee; }
int    CheckingAccount::getOverdraftCount() const { return overdraft_count; }

void CheckingAccount::setOverdraftLimit(double limit) {
    if (limit < 0.0)
        throw std::invalid_argument("Overdraft limit cannot be negative.");
    overdraft_limit = limit;
}

// ── withdraw: allows balance to go negative down to -overdraft_limit,
//             charges a flat overdraft_fee per overdraft event ────────────────
bool CheckingAccount::withdraw(double amount) {
    if (!isActive())
        throw std::runtime_error("Cannot operate on a closed account.");
    if (amount <= 0.0)
        throw std::invalid_argument("Withdrawal amount must be positive.");
    if (amount > balance + overdraft_limit) return false;

    bool is_overdraft = (amount > balance);
    balance -= amount;
    if (is_overdraft) {
        ++overdraft_count;
        balance -= overdraft_fee;   // NOTE: fee deducted from balance directly;
                                    //       NOT recorded as a separate transaction
        history.push_back({Transaction::Type::WITHDRAWAL, amount, balance,
                           currentTimestamp(),
                           "Overdraft withdrawal (fee: "
                           + std::to_string(static_cast<int>(overdraft_fee)) + ")"});
    } else {
        history.push_back({Transaction::Type::WITHDRAWAL, amount, balance,
                           currentTimestamp(), "Withdrawal"});
    }
    return true;
}

std::string CheckingAccount::getType() const { return "Checking"; }

std::string CheckingAccount::getInfo() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << Account::getInfo()                  << "\n"
        << "OD Limit   : " << overdraft_limit  << "\n"
        << "OD Fee     : " << overdraft_fee     << "\n"
        << "OD Count   : " << overdraft_count;
    return oss.str();
}

// ════════════════════════════════════════════════════════════════════════════
// Bank
// ════════════════════════════════════════════════════════════════════════════

Bank::Bank(const std::string& name) : bank_name(name), next_id(1) {
    if (name.empty())
        throw std::invalid_argument("Bank name must not be empty.");
}

int Bank::openSavingsAccount(const std::string& owner, double initial_balance,
                             double rate, double min_balance) {
    int id = next_id++;
    accounts.push_back(
        std::make_shared<SavingsAccount>(id, owner, initial_balance,
                                         rate, min_balance));
    return id;
}

int Bank::openCheckingAccount(const std::string& owner, double initial_balance,
                              double overdraft_limit_val, double fee) {
    int id = next_id++;
    accounts.push_back(
        std::make_shared<CheckingAccount>(id, owner, initial_balance,
                                          overdraft_limit_val, fee));
    return id;
}

// Returns nullptr if no account with the given ID exists.
std::shared_ptr<Account> Bank::findAccount(int id) const {
    auto it = std::find_if(accounts.begin(), accounts.end(),
        [id](const std::shared_ptr<Account>& a){ return a->getID() == id; });
    return (it != accounts.end()) ? *it : nullptr;
}

bool Bank::closeAccount(int id) {
    auto acct = findAccount(id);
    if (!acct) return false;
    acct->close();
    return true;
}

std::vector<std::shared_ptr<Account>>
Bank::getAccountsByOwner(const std::string& owner) const {
    std::vector<std::shared_ptr<Account>> result;
    for (const auto& a : accounts)
        if (a->getOwnerName() == owner) result.push_back(a);
    return result;
}

double Bank::getTotalAssets() const {
    double total = 0.0;
    for (const auto& a : accounts) total += a->getBalance();
    return total;
}

std::string Bank::getBankName()     const { return bank_name; }
int         Bank::getAccountCount() const { return static_cast<int>(accounts.size()); }

const std::vector<std::shared_ptr<Account>>& Bank::getAllAccounts() const {
    return accounts;
}
