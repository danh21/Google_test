/**
 * BankSystemTest.cpp  — Combined Edition (Revision 1 + Revision 2)
 *
 * GTest suite for BankSystem.cpp — TRAINEE EVALUATION exercise.
 *
 * Application : Personal Bank Account Management System
 * Source file : BankSystem.cpp
 * Header file : BankSystem.hpp
 *
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║  TWO EVALUATION PERSPECTIVES                                            ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║  PART A (G01–G07) — "Bug in the test logic" perspective                 ║
 * ║    Bugs are injected inside the test code itself.                       ║
 * ║    BankSystem.cpp is CORRECT.  The trainee must find and fix the        ║
 * ║    mistake the TEST AUTHOR made.                                        ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║  PART B (G08–G14) — "Tester mistake in practice" perspective            ║
 * ║    Every bug is a mistake a real tester makes while writing tests:      ║
 * ║    wrong null-checks, dangling refs, shadow variables, vacuous          ║
 * ║    assertions, infinite loops, etc.  BankSystem.cpp is still CORRECT.  ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 *
 * Trainees must for EVERY test case:
 *   1. Dry-run / read the test code carefully.
 *   2. Identify the root cause of the injected defect.
 *   3. Propose a correct fix and explain the underlying C++ mechanism.
 *
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │        PART A — Bug injected into test logic                            │
 * ├───────┬──────────────────────────────────┬──────────────────────────────┤
 * │ GROUP │ BUG TYPE                         │ TEST NAMES                   │
 * ├───────┼──────────────────────────────────┼──────────────────────────────┤
 * │  G01  │ Segmentation Fault               │ SegFault_Easy/Mid/Hard       │
 * │  G02  │ Use After Free                   │ UAF_Easy/Mid/Hard            │
 * │  G03  │ Data Corruption                  │ DataCorrupt_Easy/Mid/Hard    │
 * │  G04  │ Logic Bug (transfer/overdraft)   │ LogicBug_Easy/Mid/Hard       │
 * │  G05  │ Boundary / Validation Bug        │ Boundary_Easy/Mid/Hard       │
 * │  G06  │ Index / Memory Overrun           │ Overrun_Easy/Mid/Hard        │
 * │  G07  │ Double Operation / Stale State   │ DoubleOp_Easy/Mid/Hard       │
 * ├───────┴──────────────────────────────────┴──────────────────────────────┤
 * │        PART B — Real tester mistakes in practice                        │
 * ├───────┬──────────────────────────────────┬──────────────────────────────┤
 * │  G08  │ Segmentation Fault (tester)      │ TSegFault_Easy/Mid/Hard      │
 * │  G09  │ Use After Free (tester)          │ TUAF_Easy/Mid/Hard           │
 * │  G10  │ Memory / Index Overrun (tester)  │ TOverrun_Easy/Mid/Hard       │
 * │  G11  │ Wrong Assertion Value            │ TAssert_Easy/Mid/Hard        │
 * │  G12  │ Timeout / Infinite Loop          │ TTimeout_Easy/Mid/Hard       │
 * │  G13  │ Variable Shadowing               │ TShadow_Easy/Mid/Hard        │
 * │  G14  │ False Positive (Vacuous Test)    │ TFalsePos_Easy/Mid/Hard      │
 * └───────┴──────────────────────────────────┴──────────────────────────────┘
 *
 * Difficulty (applies to both parts):
 *   Easy   — defect is visible on first read.
 *   Medium — requires tracing data flow across 8–14 lines; verbose setup
 *            is intentional misdirection.
 *   Hard   — test appears structurally correct; defect is contextual and
 *            only visible after deep analysis of C++ semantics + domain logic.
 *
 * ── Key BankSystem.cpp behaviours to keep in mind ─────────────────────────
 *   · Bank::findAccount          → returns nullptr (NOT throws) for missing ID.
 *   · SavingsAccount::withdraw   → returns false (NOT throws) when
 *                                  balance − amount < minimum_balance.
 *   · CheckingAccount::withdraw  → on overdraft, deducts overdraft_fee from
 *                                  balance directly; fee is NOT a separate tx.
 *   · Account::transfer          → throws std::invalid_argument when src==dst.
 *   · Account::transfer          → calls VIRTUAL withdraw on the source.
 *   · Account::getTransactionHistory() → returns const& (not a copy).
 *   · SavingsAccount::applyInterest    → COMPOUNDS on current balance.
 *   · Account constructed with initial_balance == 0.0 → empty history vector.
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <numeric>

#include "BankSystem.hpp"

class BankSystemTest : public ::testing::Test {
protected:
    Bank bank{"VCB Test Bank"};

    int sid1 = -1;   // Alice    savings   balance=1000  rate=5%   min=100
    int sid2 = -1;   // Bob      savings   balance=2000  rate=4%   min=200
    int cid1 = -1;   // Carol    checking  balance=500   OD=300    fee=20
    int cid2 = -1;   // Dave     checking  balance=800   OD=400    fee=30

    void SetUp() override {
        sid1 = bank.openSavingsAccount ("Alice",  1000.0, 0.05, 100.0);
        sid2 = bank.openSavingsAccount ("Bob",    2000.0, 0.04, 200.0);
        cid1 = bank.openCheckingAccount("Carol",   500.0, 300.0, 20.0);
        cid2 = bank.openCheckingAccount("Dave",    800.0, 400.0, 30.0);
    }

    std::shared_ptr<SavingsAccount>  sa(int id) {
        return std::dynamic_pointer_cast<SavingsAccount>(bank.findAccount(id));
    }
    std::shared_ptr<CheckingAccount> ca(int id) {
        return std::dynamic_pointer_cast<CheckingAccount>(bank.findAccount(id));
    }
};

// ============================================================================
// G01. SEGMENTATION FAULT (Dereferencing Null/Invalid Pointers)
// ============================================================================
TEST_F(BankSystemTest, SegFault_Easy) {
    auto acct = bank.findAccount(sid1);
    //  auto acct = std::make_shared<SavingsAccount>(1, "temp", 1000.0, 0.03, 100.0);
    Account* raw = acct.get();

    // std::cout << acct.use_count() << std::endl; //2

    acct.reset();
    raw = nullptr;  /* should reset pointer after use. */

    // std::cout << acct.use_count() << std::endl; //0

    /* checkpoint */
    EXPECT_DOUBLE_EQ(raw->getBalance(), 1000.0); 
}

TEST_F(BankSystemTest, SegFault_Mid) {
    std::vector<int> process_ids = {sid1, sid2, 9999, cid1, cid2};

    double total_after_bonus = 0.0;

    for (int id : process_ids) {
        auto acct = bank.findAccount(id);   // ID 9999 not exist, findAccount returns nullptr
        acct->deposit(50.0); 
        total_after_bonus += acct->getBalance();
    }
}

TEST_F(BankSystemTest, SegFault_Hard) {
    const auto& all_accts = bank.getAllAccounts();
    const std::shared_ptr<Account>* sp_elem_ptr = &all_accts[0];

    // std::cout << &all_accts[0] << std::endl;
    // std::cout << &bank.getAllAccounts()[0] << std::endl;

    // push_back activates vector realloc, old region of all_accts is released which makes sp_elem_ptr is stale.
    for (int i = 0; i < 1; ++i) {
        bank.openSavingsAccount("BatchUser" + std::to_string(i), 500.0 + i, 0.03, 100.0);
    }

    // std::cout << &all_accts[0] << std::endl;
    // std::cout << &bank.getAllAccounts()[0] << std::endl;

    EXPECT_EQ((*sp_elem_ptr)->getID(), 1); 
}

// ============================================================================
// G02. USE AFTER FREE (Dangling References/Iterators)
// ============================================================================
TEST_F(BankSystemTest, UAF_Easy) {
    auto acct = std::make_shared<SavingsAccount>(99, "Temp", 500.0, 0.05, 100.0);
    const auto& hist_ref = acct->getTransactionHistory(); 

    // acct.reset() destroys the Account object, which also destroys the history vector,
    // leaving hist_ref as a dangling reference.
    acct.reset();

    EXPECT_EQ(hist_ref.size(), 1u); 
}

TEST_F(BankSystemTest, UAF_Mid) {
    auto alice = sa(sid1);
    const auto& hist_ref = alice->getTransactionHistory();
    const Transaction* first_tx = &hist_ref[0]; 

    // push_back makes 'history' vector reallocate, which makes first_tx (points to old buffer) is wrong
    for (int i = 0; i < 50; ++i) alice->deposit(20.0);
    alice->applyInterest();
    for (int i = 0; i < 20; ++i) ASSERT_TRUE(alice->withdraw(5.0));

    EXPECT_DOUBLE_EQ(first_tx->amount, 1000.0);
    // EXPECT_DOUBLE_EQ(hist_ref[0].amount, 1000.0);    //fix issue
}

TEST_F(BankSystemTest, UAF_Hard) {
    auto alice = sa(sid1);
    const auto& hist_ref = alice->getTransactionHistory();
    const Transaction* tx_first = &hist_ref.front();
    const Transaction* tx_last  = &hist_ref.back();

    // push_back makes 'history' vector reallocate, which makes pointers tx_first and tx_last (points to old buffer) are wrong
    for (int i = 0; i < 60; ++i) alice->deposit(10.0);
    for (int i = 0; i < 30; ++i) ASSERT_TRUE(alice->withdraw(5.0));

    EXPECT_DOUBLE_EQ(tx_first->amount, 1000.0); 
    // EXPECT_DOUBLE_EQ(hist_ref.front().amount, 1000.0); // fix issue
}

// ============================================================================
// G03. DATA CORRUPTION (Logical & Calculation Errors)
// ============================================================================
TEST_F(BankSystemTest, DataCorrupt_Easy) {
    auto alice = bank.findAccount(sid1); 
    auto carol = bank.findAccount(cid1); 
    ASSERT_TRUE(alice->transfer(200.0, *carol));

    EXPECT_DOUBLE_EQ(alice->getBalance(), 1200.0);  //800
    EXPECT_DOUBLE_EQ(carol->getBalance(),  300.0);  //700
}

TEST_F(BankSystemTest, DataCorrupt_Mid) {
    auto alice = sa(sid1);

    // setInterestRate(5) ~ 500% 
    alice->setInterestRate(5);

    // expect 0.05 ~ 5%.
    // alice->setInterestRate(0.05);   //fix issue

    double total_interest = 0.0;
    for (int month = 1; month <= 3; ++month) {
        total_interest += alice->applyInterest();
    }

    EXPECT_DOUBLE_EQ(total_interest, 157.63); 
}

TEST_F(BankSystemTest, DataCorrupt_Hard) {
    CheckingAccount carol2(10, "Carol2", 300.0, 200.0, 25.0);
    carol2.withdraw(350.0); // withdraw exceeds balance, causes overdraft -> detect overdraft fee issue

    double deposited = carol2.getTotalDeposited();
    double withdrawn = carol2.getTotalWithdrawn();
    // double overdraft_fee_total = carol2.getOverdraftCount() * carol2.getOverdraftFee();  //fix issue
    
    EXPECT_DOUBLE_EQ(deposited - withdrawn, carol2.getBalance()); 
    // EXPECT_DOUBLE_EQ(deposited - withdrawn - overdraft_fee_total, carol2.getBalance()); //fix issue
}

// ============================================================================
// G04. LOGIC BUG (Tester Logic Mistakes)
// ============================================================================
TEST_F(BankSystemTest, LogicBug_Easy) {
    auto carol = ca(cid1); 
    ASSERT_TRUE(carol->withdraw(600.0)); // balance 500, fee 20

    EXPECT_DOUBLE_EQ(carol->getBalance(), -100.0); //500 - 600 - 20 = -120.0.
}

TEST_F(BankSystemTest, LogicBug_Mid) {
    auto alice = bank.findAccount(sid1);

    EXPECT_NO_THROW(alice->transfer(100.0, *alice)); //Account::transfer throws std::invalid_argument if source == target.
}

TEST_F(BankSystemTest, LogicBug_Hard) {
    auto a = bank.findAccount(sid1); 
    auto b = bank.findAccount(sid2); // 2000
    auto c = bank.findAccount(cid1); // 500
    a->transfer(200.0, *b); //+200
    b->transfer(300.0, *c); //-300
    c->transfer(100.0, *a); 
    b->transfer(100.0, *a); //-100

    ASSERT_DOUBLE_EQ(b->getBalance(), 1600.0); //2000 + 200 - 300 - 100 = 1800
    EXPECT_DOUBLE_EQ(c->getBalance(), 800.0);  //500 + 300 - 100 = 700
    std::cout << "B balance: " << b->getBalance();
}

// // ============================================================================
// // G05. BOUNDARY / VALIDATION BUG (Lỗi kiểm tra biên và hợp lệ dữ liệu)
// // ============================================================================
// TEST_F(BankSystemTest, Boundary_Easy) {
//     auto alice = bank.findAccount(sid1);

//     EXPECT_NO_THROW(alice->deposit(-100.0)); 
// }

// TEST_F(BankSystemTest, Boundary_Mid) {
//     auto alice = sa(sid1); 
//     alice->withdraw(900.0); 

//     ASSERT_TRUE(alice->withdraw(1.0)); 
// }

// TEST_F(BankSystemTest, Boundary_Hard) {
//     auto carol = ca(cid1); 
//     carol->withdraw(600.0); 
//     carol->deposit(300.0);  
//     carol->withdraw(230.0); 

//     EXPECT_DOUBLE_EQ(carol->getBalance(), 30.0 - 80.0); 
// }

// // ============================================================================
// // G06. INDEX / MEMORY OVERRUN (Lỗi tràn mảng và truy cập ngoài biên)
// // ============================================================================
// TEST_F(BankSystemTest, Overrun_Easy) {
//     SavingsAccount empty_acct(50, "Ghost", 0.0, 0.05, 0.0);
//     const auto& hist = empty_acct.getTransactionHistory();

//     EXPECT_DOUBLE_EQ(hist[0].amount, 0.0); 
// }

// TEST_F(BankSystemTest, Overrun_Mid) {
//     auto alice = sa(sid1);
//     const auto& hist = alice->getTransactionHistory();

//     for (size_t i = 0; i <= hist.size(); ++i) {
//         EXPECT_GT(hist[i].amount, 0.0);
//     }
// }

// TEST_F(BankSystemTest, Overrun_Hard) {
//     auto snapshot = alice->getTransactionHistory(); 
//     alice->deposit(100.0); 

//     for (size_t i = 0; i < live_hist.size(); ++i) {
//         EXPECT_DOUBLE_EQ(snapshot[i].amount, live_hist[i].amount);
//     }
// }

// // ============================================================================
// // G07. DOUBLE OPERATION / STALE SNAPSHOT (Lỗi lặp thao tác & Dữ liệu cũ)
// // ============================================================================
// TEST_F(BankSystemTest, DoubleOp_Easy) {
//     auto alice = bank.findAccount(sid1);
//     alice->close();

//     EXPECT_NO_THROW(alice->close()); 
// }

// TEST_F(BankSystemTest, DoubleOp_Mid) {
//     auto alice = sa(sid1);
//     double first_credit = alice->applyInterest(); 

//     for (int month = 2; month <= 3; ++month) {
//         double credit = alice->applyInterest();
//         EXPECT_DOUBLE_EQ(credit, first_credit); 
//     }
// }

// TEST_F(BankSystemTest, DoubleOp_Hard) {
//     double assets_before = bank.getTotalAssets(); 

//     EXPECT_DOUBLE_EQ(bank.getTotalAssets(), assets_before + external_total); 
// }

// // ============================================================================
// // G08. TESTER MISTAKES (Sai sót của Tester khi viết code)
// // ============================================================================
// TEST_F(BankSystemTest, TSegFault_Easy) {
//     auto ghost = bank.findAccount(9999); 

//     ghost->deposit(100.0); 
// }

// TEST_F(BankSystemTest, TSegFault_Mid) {
//     auto handle = std::make_shared<SavingsAccount>(99, "Isolated", 800.0);
//     Account* raw = handle.get();

//     handle.reset(); 
//     EXPECT_DOUBLE_EQ(raw->getBalance(), 800.0); 
// }

// TEST_F(BankSystemTest, TSegFault_Hard) {
//     const std::shared_ptr<Account>* elem0_ptr = &all[0]; 
//     for (int i = 0; i < 200; ++i) bank.openSavingsAccount("New", 500.0); 

//     EXPECT_EQ((*elem0_ptr)->getID(), expected_id); 
// }

// // ============================================================================
// // G09. USE AFTER FREE CAUSED BY THE TESTER (Lỗi giải phóng vùng nhớ sớm)
// // ============================================================================
// TEST_F(BankSystemTest, TUAF_Easy) {

//     const auto& hist_ref = std::make_shared<SavingsAccount>(77, "Tmp", 600.0)->getTransactionHistory();

//     EXPECT_EQ(hist_ref.size(), 1u); 
// }

// TEST_F(BankSystemTest, TUAF_Mid) {
//     auto alice = sa(sid1);
//     const auto& hist_ref = alice->getTransactionHistory();
//     const Transaction* first_entry = &hist_ref[0]; 

//     for (int i = 0; i < 30; ++i) alice->deposit(10.0);

//     EXPECT_EQ(first_entry->description, "Initial deposit"); 
// }

// TEST_F(BankSystemTest, TUAF_Hard) {
//     std::vector<std::shared_ptr<Account>> candidates = bank.getAllAccounts();

//     for (const auto& acct : candidates) {
//         if (acct->getBalance() < 100.0) {
//             candidates.erase(std::find(candidates.begin(), candidates.end(), acct)); 
//         }
//     }
// }

// // ============================================================================
// // G10. MEMORY / INDEX OVERRUN BY TESTER (Lỗi tràn mảng do tester viết sai)
// // ============================================================================
// TEST_F(BankSystemTest, TOverrun_Easy) {
//     auto ghost_accts = bank.getAccountsByOwner("Nonexistent");

//     EXPECT_EQ(ghost_accts[0]->getOwnerName(), "Nonexistent"); 
// }

// TEST_F(BankSystemTest, TOverrun_Mid) {
//     const auto& hist = alice->getTransactionHistory();

//     for (size_t i = 0; i <= hist.size(); ++i) { 
//         EXPECT_GT(hist[i].amount, 0.0);
//     }
// }

// TEST_F(BankSystemTest, TOverrun_Hard) {
//     auto snapshot = alice->getTransactionHistory(); 

//     for (size_t i = 0; i < live_hist.size(); ++i) {
//         if (snapshot[i].amount != live_hist[i].amount) { 

//         }
//     }
// }

// // ============================================================================
// // G11. WRONG ASSERTION VALUE (Sai lệch giá trị kỳ vọng do tester tính sai)
// // ============================================================================
// TEST_F(BankSystemTest, TAssert_Easy) {
//     auto carol = ca(cid1); 
//     carol->withdraw(600.0);

//     EXPECT_DOUBLE_EQ(carol->getBalance(), -100.0); 
// }

// TEST_F(BankSystemTest, TAssert_Mid) {
//     alice->transfer(700.0, *bob);

//     EXPECT_DOUBLE_EQ(bank.findAccount(sid1)->getBalance(), 600.0); 
//     EXPECT_DOUBLE_EQ(bank.findAccount(sid1)->getBalance(), 3100.0); 
// }

// TEST_F(BankSystemTest, TAssert_Hard) {

//     double expected_final = principal + principal * bob->getInterestRate() * 4;
//     EXPECT_DOUBLE_EQ(bob->getBalance(), expected_final); 
// }

// // ============================================================================
// // G12. TIMEOUT / INFINITE LOOP (Vòng lặp vô tận khiến test bị treo)
// // ============================================================================
// TEST_F(BankSystemTest, TTimeout_Easy) {

//     while (alice->withdraw(10.0)) {
//         alice->deposit(1.0); 
//     }
// }

// TEST_F(BankSystemTest, TTimeout_Mid) {

//     for (int id : opened_ids) {
//         auto same_owner = bank.getAccountsByOwner(acct->getOwnerName()); 
//     }
// }

// TEST_F(BankSystemTest, TTimeout_Hard) {

//     for (int i = 0; i <= steps; ++i) {
//         alice->withdraw(step_amount);
//         alice->deposit(0.5); 
//     }
// }

// // ============================================================================
// // G13. VARIABLE SHADOWING (Lỗi che khuất biến/Khai báo trùng tên)
// // ============================================================================
// TEST_F(BankSystemTest, TShadow_Easy) {

//     int sid1 = bank.openSavingsAccount("ShadowUser", 500.0, 0.03, 50.0);

//     auto acct = bank.findAccount(sid1); 
//     EXPECT_EQ(acct->getOwnerName(), "Alice"); 
// }

// TEST_F(BankSystemTest, TShadow_Mid) {
//     int id = sid1; 

//     for (int i = 0; i < 50; ++i) {
//         id = bank.openSavingsAccount("BatchUser" + std::to_string(i), 200.0);
//     }

//     bank.findAccount(id)->deposit(500.0);
//     EXPECT_EQ(bank.findAccount(id)->getOwnerName(), "Alice"); 
// }

// TEST_F(BankSystemTest, TShadow_Hard) {
//     bank.closeAccount(sid1); 

//     int sid1 = bank.openSavingsAccount("NewAlice", 1500.0);

//     EXPECT_TRUE(bank.findAccount(1)->isActive()); 
// }

// // ============================================================================
// // G14. FALSE POSITIVE / VACUOUS TEST (Test luôn vượt qua nhưng không có giá trị)
// // ============================================================================
// TEST_F(BankSystemTest, TFalsePos_Easy) {
//     auto alice = bank.findAccount(sid1);
//     double amount = 250.0;
//     alice->deposit(amount);

//     EXPECT_GT(alice->getBalance(), 0.0); 
//     EXPECT_NE(alice->getBalance(), balance_before); 
// }

// TEST_F(BankSystemTest, TFalsePos_Mid) {

//     alice->transfer(400.0, *bob);
//     EXPECT_DOUBLE_EQ(bank.getTotalAssets(), 5400.0); 
// }

// TEST_F(BankSystemTest, TFalsePos_Hard) {

//     for (int i = 1; i <= 10; ++i) {
//         acct->deposit(withdraw_amount + 50.0); 
//         acct->withdraw(withdraw_amount);
//     }
//     EXPECT_EQ(acct->getOverdraftCount(), 0); 
// }