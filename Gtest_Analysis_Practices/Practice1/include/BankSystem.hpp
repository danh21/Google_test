#ifndef BANKSYSTEM_HPP
#define BANKSYSTEM_HPP

/**
 * BankSystem.hpp
 *
 * Personal Bank Account Management System
 * Classes: Transaction (struct) · Account (abstract base)
 *          SavingsAccount · CheckingAccount · Bank
 */

#include <string>
#include <vector>
#include <memory>

// ─── Transaction record ──────────────────────────────────────────────────────
struct Transaction {
    enum class Type { DEPOSIT, WITHDRAWAL, TRANSFER_IN, TRANSFER_OUT, INTEREST };
    Type        type;
    double      amount;
    double      balance_after;
    std::string timestamp;
    std::string description;
};

// ─── Account (abstract base) ─────────────────────────────────────────────────
class Account {
protected:
    int                      account_id;
    std::string              owner_name;
    double                   balance;
    bool                     is_active;
    std::vector<Transaction> history;

public:
    Account(int id, const std::string& owner, double initial_balance);
    virtual ~Account();

    // Getters
    int                              getID()                 const;
    std::string                      getOwnerName()          const;
    double                           getBalance()            const;
    bool                             isActive()              const;
    const std::vector<Transaction>&  getTransactionHistory() const;
    int                              getTransactionCount()   const;

    // Setter
    void setOwnerName(const std::string& new_name);

    // Core operations
    void         deposit(double amount);
    virtual bool withdraw(double amount);
    bool         transfer(double amount, Account& target);
    void         close();
    void         reactivate();

    // Aggregates
    double      getTotalDeposited() const;
    double      getTotalWithdrawn() const;
    std::string getStatementSummary() const;

    // Polymorphic interface
    virtual std::string getType() const = 0;
    virtual std::string getInfo() const;
};

// ─── SavingsAccount ───────────────────────────────────────────────────────────
class SavingsAccount : public Account {
private:
    double interest_rate;    // annual rate as decimal  (e.g. 0.05 = 5 %)
    double minimum_balance;  // balance must not fall below this after withdrawal

public:
    SavingsAccount(int id, const std::string& owner, double initial_balance,
                   double interest_rate  = 0.05,
                   double min_balance    = 100.0);

    double getInterestRate()    const;
    double getMinimumBalance()  const;
    void   setInterestRate(double rate);

    double      applyInterest();             // credits interest, returns amount
    bool        withdraw(double amount) override;
    std::string getType()               const override;
    std::string getInfo()               const override;
};

// ─── CheckingAccount ─────────────────────────────────────────────────────────
class CheckingAccount : public Account {
private:
    double overdraft_limit;   // how far below zero the balance may go
    double overdraft_fee;     // flat fee charged per overdraft event
    int    overdraft_count;   // total overdraft events so far

public:
    CheckingAccount(int id, const std::string& owner, double initial_balance,
                    double overdraft_limit = 500.0,
                    double overdraft_fee   = 25.0);

    double getOverdraftLimit()  const;
    double getOverdraftFee()    const;
    int    getOverdraftCount()  const;
    void   setOverdraftLimit(double limit);

    bool        withdraw(double amount) override;
    std::string getType()               const override;
    std::string getInfo()               const override;
};

// ─── Bank ─────────────────────────────────────────────────────────────────────
class Bank {
private:
    std::string                            bank_name;
    std::vector<std::shared_ptr<Account>>  accounts;
    int                                    next_id;

public:
    explicit Bank(const std::string& name);

    int openSavingsAccount (const std::string& owner, double initial_balance,
                            double rate        = 0.05,
                            double min_balance = 100.0);
    int openCheckingAccount(const std::string& owner, double initial_balance,
                            double overdraft_limit = 500.0,
                            double fee             = 25.0);

    std::shared_ptr<Account> findAccount(int id) const;
    bool closeAccount(int id);

    std::vector<std::shared_ptr<Account>> getAccountsByOwner(
        const std::string& owner) const;

    double      getTotalAssets()  const;
    std::string getBankName()     const;
    int         getAccountCount() const;
    const std::vector<std::shared_ptr<Account>>& getAllAccounts() const;
};

#endif // BANKSYSTEM_HPP
