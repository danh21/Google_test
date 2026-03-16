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

// ─── Shared fixture ───────────────────────────────────────────────────────────
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

// [Dễ] Dangling raw pointer sau khi shared_ptr sở hữu bị hủy.
TEST_F(BankSystemTest, SegFault_Easy) {
    auto acct = bank.findAccount(sid1);
    Account* raw = acct.get();

    // Rootcause: acct.reset() hủy đối tượng Account, khiến raw trỏ vào vùng nhớ đã giải phóng.
    // Fix: Chỉ reset shared_ptr sau khi đã hoàn thành việc sử dụng raw pointer.
    acct.reset();

    EXPECT_DOUBLE_EQ(raw->getBalance(), 1000.0); 
}

// [Trung bình] Dereference nullptr do không kiểm tra kết quả trả về từ hàm tìm kiếm.
TEST_F(BankSystemTest, SegFault_Mid) {
    std::vector<int> process_ids = {sid1, sid2, 9999, cid1, cid2};

    // Rootcause: ID 9999 không tồn tại, findAccount trả về nullptr thay vì ném ngoại lệ.
    // Fix: Thêm check `if (acct)` hoặc sử dụng ASSERT_NE(acct, nullptr) trước khi gọi các hàm thành viên.
    for (int id : process_ids) {
        auto acct = bank.findAccount(id);
        acct->deposit(50.0); // Segfault tại id=9999
        total_after_bonus += acct->getBalance();
    }
}


// [Khó] Dangling pointer trỏ vào buffer của vector bị vô hiệu hóa sau khi Reallocation.
TEST_F(BankSystemTest, SegFault_Hard) {
    const auto& all_accts = bank.getAllAccounts();
    const std::shared_ptr<Account>* sp_elem_ptr = &all_accts[0]; 

    // Rootcause: Mở 200 accounts kích hoạt vector realloc, vùng nhớ cũ của all_accts bị giải phóng làm sp_elem_ptr bị stale.
    // Fix: Sử dụng index (all_accts[0]) thay vì lưu địa chỉ của phần tử bên trong vector.
    for (int i = 0; i < 200; ++i) {
        bank.openSavingsAccount("BatchUser" + std::to_string(i), 500.0 + i, 0.03, 100.0);
    }

    EXPECT_EQ((*sp_elem_ptr)->getID(), 1); 
}

// ============================================================================
// G02. USE AFTER FREE (Dangling References/Iterators)
// ============================================================================

// [Dễ] Truy cập tham chiếu tới thành viên của đối tượng đã bị tiêu hủy.
TEST_F(BankSystemTest, UAF_Easy) {
    auto acct = std::make_shared<SavingsAccount>(99, "Temp", 500.0, 0.05, 100.0);
    const auto& hist_ref = acct->getTransactionHistory(); 

    // Rootcause: acct.reset() hủy đối tượng Account kéo theo vector history, khiến hist_ref lơ lửng.
    // Fix: Duy trì lifetime của đối tượng chủ quản (acct) cho đến khi sử dụng xong reference của nó.
    acct.reset();

    EXPECT_EQ(hist_ref.size(), 1u); 
}

// [Trung bình] Con trỏ trỏ vào phần tử vector bị vô hiệu hóa khi vector tăng kích thước.
TEST_F(BankSystemTest, UAF_Mid) {
    auto alice = sa(sid1);
    const auto& hist_ref = alice->getTransactionHistory();
    const Transaction* first_tx = &hist_ref[0]; 

    // Rootcause: 72 thao tác push_back làm history vector reallocate, khiến first_tx (trỏ vào buffer cũ) bị sai.
    // Fix: Truy cập phần tử trực tiếp qua `hist_ref[0]` thay vì dùng con trỏ thô lưu địa chỉ phần tử.
    for (int i = 0; i < 50; ++i) alice->deposit(20.0);
    alice->applyInterest();
    for (int i = 0; i < 20; ++i) ASSERT_TRUE(alice->withdraw(5.0));

    EXPECT_DOUBLE_EQ(first_tx->amount, 1000.0); 
}

// [Khó] Con trỏ đầu (front) và cuối (back) bị vô hiệu hóa sau chuỗi thao tác dài.
TEST_F(BankSystemTest, UAF_Hard) {
    auto alice = sa(sid1);
    const auto& hist_ref = alice->getTransactionHistory();
    const Transaction* tx_first = &hist_ref.front();
    const Transaction* tx_last  = &hist_ref.back();

    // Rootcause: 90 thao tác đảm bảo vector history cấp phát lại vùng nhớ, làm cả tx_first và tx_last bị dangling.
    // Fix: Luôn truy cập phần tử qua index hoặc iterator từ live vector sau khi có thao tác thay đổi số lượng phần tử.
    for (int i = 0; i < 60; ++i) alice->deposit(10.0);
    for (int i = 0; i < 30; ++i) ASSERT_TRUE(alice->withdraw(5.0));

    EXPECT_DOUBLE_EQ(tx_first->amount, 1000.0); 
}

// ============================================================================
// G03. DATA CORRUPTION (Logical & Calculation Errors)
// ============================================================================

// [Dễ] Nhầm lẫn hướng chuyển tiền (Source vs Target).
TEST_F(BankSystemTest, DataCorrupt_Easy) {
    auto alice = bank.findAccount(sid1); // 1000
    auto carol = bank.findAccount(cid1); // 500
    ASSERT_TRUE(alice->transfer(200.0, *carol));

    // Rootcause: Khẳng định sai thực tế nghiệp vụ: Chuyển đi thì số dư giảm, nhận thì số dư tăng.
    // Fix: Sửa lại kỳ vọng: Alice là 800.0, Carol là 700.0.
    EXPECT_DOUBLE_EQ(alice->getBalance(), 1200.0); 
    EXPECT_DOUBLE_EQ(carol->getBalance(),  300.0); 
}

// [Trung bình] Sai lệch quy mô lãi suất (Tỉ lệ phần trăm nguyên vs Thập phân).
TEST_F(BankSystemTest, DataCorrupt_Mid) {
    auto alice = sa(sid1);
    // Rootcause: setInterestRate(5) tương đương 500% lãi suất. Implementation mong đợi giá trị thập phân (0.05 = 5%).
    // Fix: Truyền giá trị đúng là 0.05.
    alice->setInterestRate(5);

    double total_interest = 0.0;
    for (int month = 1; month <= 3; ++month) {
        total_interest += alice->applyInterest();
    }
    EXPECT_DOUBLE_EQ(total_interest, 150.0); // Thực tế sẽ ra ~15,750
}

// [Khó] Lệ phí thấu chi (Overdraft Fee) không hiển thị trong Transaction History.
TEST_F(BankSystemTest, DataCorrupt_Hard) {
    CheckingAccount carol2(10, "Carol2", 300.0, 200.0, 25.0);
    carol2.withdraw(50.0); // Gây thấu chi

    // Rootcause: Phí thấu chi được trừ thẳng vào số dư nhưng KHÔNG tạo record giao dịch riêng biệt.
    // Fix: Công thức kiểm chứng phải là: Số dư = Tổng nạp - Tổng rút - (Số lần thấu chi * Lệ phí thấu chi).
    double deposited = carol2.getTotalDeposited();
    double withdrawn = carol2.getTotalWithdrawn();
    EXPECT_DOUBLE_EQ(deposited - withdrawn, carol2.getBalance()); 
}

// ============================================================================
// G04. LOGIC BUG (Tester Logic Mistakes)
// ============================================================================

// [Dễ] Quên trừ lệ phí thấu chi trong giá trị kỳ vọng.
TEST_F(BankSystemTest, LogicBug_Easy) {
    auto carol = ca(cid1); // balance 500, fee 20
    ASSERT_TRUE(carol->withdraw(600.0));

    // Rootcause: Rút tiền thấu chi sẽ trừ đồng thời cả số tiền rút và lệ phí (20.0).
    // Fix: Giá trị kỳ vọng đúng phải là 500 - 600 - 20 = -120.0.
    EXPECT_DOUBLE_EQ(carol->getBalance(), -100.0); 
}

// [Trung bình] Mong đợi hàm không ném lỗi trong khi logic nghiệp vụ bắt buộc phải throw.
TEST_F(BankSystemTest, LogicBug_Mid) {
    auto alice = bank.findAccount(sid1);
    // Rootcause: Account::transfer ném std::invalid_argument nếu source == target. Tester dùng EXPECT_NO_THROW gây Error.
    // Fix: Sử dụng EXPECT_THROW(..., std::invalid_argument).
    EXPECT_NO_THROW(alice->transfer(100.0, *alice)); 
}

// [Khó] Sai sót trong tính toán "Zero-sum" sau chuỗi giao dịch phức tạp.
TEST_F(BankSystemTest, LogicBug_Hard) {
    auto a = bank.findAccount(sid1); // 1000
    auto b = bank.findAccount(sid2); // 2000
    auto c = bank.findAccount(cid1); // 500
    a->transfer(200.0, *b); // B nhận 200
    b->transfer(300.0, *c); // B gửi 300
    c->transfer(100.0, *a);
    b->transfer(100.0, *a); // B gửi 100

    // Rootcause: Tester quên cộng dồn khoản nhận +200 ở bước đầu cho tài khoản B và khoản gửi -100 cho C.
    // Fix: Tính toán lại đúng dòng tiền: B = 2000 + 200 - 300 - 100 = 1800.
    EXPECT_DOUBLE_EQ(b->getBalance(), 1600.0); 
    EXPECT_DOUBLE_EQ(c->getBalance(), 800.0);  
}

// ============================================================================
// G05. BOUNDARY / VALIDATION BUG (Lỗi kiểm tra biên và hợp lệ dữ liệu)
// ============================================================================

// [Dễ] Mong đợi nạp tiền số âm thành công trong khi hệ thống ném ngoại lệ.
TEST_F(BankSystemTest, Boundary_Easy) {
    auto alice = bank.findAccount(sid1);

    // Rootcause: deposit(-100.0) ném ra std::invalid_argument. Việc dùng EXPECT_NO_THROW khiến ngoại lệ không được bắt, làm test bị ERROR.
    // Fix: Sử dụng EXPECT_THROW(alice->deposit(-100.0), std::invalid_argument);
    EXPECT_NO_THROW(alice->deposit(-100.0)); 
}

// [Trung bình] Vi phạm số dư tối thiểu (Minimum Balance) của tài khoản tiết kiệm.
TEST_F(BankSystemTest, Boundary_Mid) {
    auto alice = sa(sid1); // balance=1000, min=100
    alice->withdraw(900.0); // balance hiện tại đúng bằng 100.

    // Rootcause: Rút thêm 1.0 sẽ làm balance = 99 < 100 (vi phạm biên). Hàm trả về false thay vì true.
    // Fix: Thay đổi kỳ vọng thành ASSERT_FALSE(alice->withdraw(1.0));
    ASSERT_TRUE(alice->withdraw(1.0)); 
}

// [Khó] Quên tính toán lệ phí thấu chi lần thứ hai khi balance âm liên tục.
TEST_F(BankSystemTest, Boundary_Hard) {
    auto carol = ca(cid1); // balance 500, phí OD 20
    carol->withdraw(600.0); // OD lần 1: balance = -120
    carol->deposit(300.0);  // balance = 180
    carol->withdraw(230.0); // balance = -50, kích hoạt thấu chi lần 2.

    // Rootcause: Tester chỉ trừ lệ phí thấu chi lần đầu, quên mất lệ phí lần 2 (20đ) khiến số dư kỳ vọng bị lệch.
    // Fix: Số dư đúng phải là -70.0 (30 - 80 - 20).
    EXPECT_DOUBLE_EQ(carol->getBalance(), 30.0 - 80.0); 
}

// ============================================================================
// G06. INDEX / MEMORY OVERRUN (Lỗi tràn mảng và truy cập ngoài biên)
// ============================================================================

// [Dễ] Truy cập phần tử đầu tiên của một vector rỗng.
TEST_F(BankSystemTest, Overrun_Easy) {
    SavingsAccount empty_acct(50, "Ghost", 0.0, 0.05, 0.0);
    const auto& hist = empty_acct.getTransactionHistory();

    // Rootcause: Tài khoản nạp 0 đồng sẽ không có lịch sử giao dịch khởi tạo (vector rỗng). hist[0] gây Undefined Behavior.
    // Fix: Luôn kiểm tra `if (!hist.empty())` hoặc `ASSERT_GT(hist.size(), 0u)` trước khi truy cập.
    EXPECT_DOUBLE_EQ(hist[0].amount, 0.0); 
}

// [Trung bình] Lỗi Off-by-one trong vòng lặp (Duyệt quá giới hạn mảng).
TEST_F(BankSystemTest, Overrun_Mid) {
    auto alice = sa(sid1);
    const auto& hist = alice->getTransactionHistory();

    // Rootcause: Dấu `<=` khiến vòng lặp truy cập đến index `hist.size()`, vượt quá biên cho phép của vector (0 đến size-1).
    // Fix: Sửa điều kiện dừng thành `i < hist.size()`.
    for (size_t i = 0; i <= hist.size(); ++i) {
        EXPECT_GT(hist[i].amount, 0.0);
    }
}

// [Khó] Truy cập index của bản copy snapshot nhưng dùng kích thước của live vector.
TEST_F(BankSystemTest, Overrun_Hard) {
    auto snapshot = alice->getTransactionHistory(); // Có N phần tử
    alice->deposit(100.0); // Live vector tăng lên N+1 phần tử.

    // Rootcause: Vòng lặp duyệt theo kích thước live vector nhưng lại truy cập index vào bản snapshot cũ (nhỏ hơn).
    // Fix: Chỉ duyệt vòng lặp đến `snapshot.size()`.
    for (size_t i = 0; i < live_hist.size(); ++i) {
        EXPECT_DOUBLE_EQ(snapshot[i].amount, live_hist[i].amount);
    }
}

// ============================================================================
// G07. DOUBLE OPERATION / STALE SNAPSHOT (Lỗi lặp thao tác & Dữ liệu cũ)
// ============================================================================

// [Dễ] Thực hiện đóng tài khoản hai lần (Double Close).
TEST_F(BankSystemTest, DoubleOp_Easy) {
    auto alice = bank.findAccount(sid1);
    alice->close();

    // Rootcause: Đóng tài khoản đã đóng sẽ ném ra std::runtime_error. Tester không bắt lỗi này làm crash test.
    // Fix: Sử dụng EXPECT_THROW(alice->close(), std::runtime_error);
    EXPECT_NO_THROW(alice->close()); 
}

// [Trung bình] Kỳ vọng lãi suất cộng dồn (Compound) giống như lãi suất phẳng (Flat).
TEST_F(BankSystemTest, DoubleOp_Mid) {
    auto alice = sa(sid1);
    double first_credit = alice->applyInterest(); // Lãi lần 1

    // Rootcause: Hệ thống tính lãi kép dựa trên số dư mới. Các lần sau tiền lãi sẽ tăng dần, không bằng `first_credit`.
    // Fix: Tính toán lại giá trị lãi kỳ vọng cho từng tháng dựa trên số dư thực tế tại thời điểm đó.
    for (int month = 2; month <= 3; ++month) {
        double credit = alice->applyInterest();
        EXPECT_DOUBLE_EQ(credit, first_credit); 
    }
}

// [Khó] Lấy snapshot tổng tài sản quá sớm (Stale Snapshot).
TEST_F(BankSystemTest, DoubleOp_Hard) {
    double assets_before = bank.getTotalAssets(); // Lấy trước khi tính lãi

    // Rootcause: Thao tác applyInterest sinh thêm tiền lãi (money out of thin air), làm tăng tổng tài sản của ngân hàng.
    // Fix: Cộng thêm tổng tiền lãi đã phát sinh vào công thức kiểm tra `assets_before + external_total + total_interest`.
    EXPECT_DOUBLE_EQ(bank.getTotalAssets(), assets_before + external_total); 
}

// ============================================================================
// G08. TESTER MISTAKES (Sai sót của Tester khi viết code)
// ============================================================================

// [Dễ] Quên kiểm tra NULL sau khi tìm kiếm tài khoản.
TEST_F(BankSystemTest, TSegFault_Easy) {
    auto ghost = bank.findAccount(9999); 

    // Rootcause: Tester giả định findAccount luôn tìm thấy hoặc ném lỗi. Thực tế nó trả về nullptr cho ID lạ.
    // Fix: Luôn thêm `ASSERT_NE(ghost, nullptr);` trước khi sử dụng.
    ghost->deposit(100.0); 
}

// [Trung bình] Giải phóng Shared pointer địa phương khiến Raw pointer bị dangling.
TEST_F(BankSystemTest, TSegFault_Mid) {
    auto handle = std::make_shared<SavingsAccount>(99, "Isolated", 800.0);
    Account* raw = handle.get();

    // Rootcause: `handle.reset()` hủy đối tượng vì đây là reference duy nhất tại chỗ. Raw pointer mất vùng nhớ trỏ tới.
    // Fix: Không reset handle cho đến khi hoàn thành các lệnh kiểm tra với biến `raw`.
    handle.reset(); 
    EXPECT_DOUBLE_EQ(raw->getBalance(), 800.0); 
}


// [Khó] Lưu địa chỉ phần tử vector rồi thực hiện thao tác làm vector Reallocate.
TEST_F(BankSystemTest, TSegFault_Hard) {
    const std::shared_ptr<Account>* elem0_ptr = &all[0]; 
    for (int i = 0; i < 200; ++i) bank.openSavingsAccount("New", 500.0); // Gây realloc

    // Rootcause: Tester nhầm lẫn giữa địa chỉ của Vector (cố định) và địa chỉ phần tử bên trong (thay đổi khi realloc).
    // Fix: Sử dụng index `all[0]` để truy cập thay vì dùng con trỏ trỏ trực tiếp vào buffer nội bộ của vector.
    EXPECT_EQ((*elem0_ptr)->getID(), expected_id); 
}

// ============================================================================
// G09. USE AFTER FREE CAUSED BY THE TESTER (Lỗi giải phóng vùng nhớ sớm)
// ============================================================================

// [Dễ] Gán tham chiếu const vào một đối tượng tạm thời (Temporary object).
TEST_F(BankSystemTest, TUAF_Easy) {
    // Rootcause: hist_ref tham chiếu đến vector của một shared_ptr tạm thời. Shared_ptr này bị hủy ngay sau dấu ';'.
    // Fix: Lưu shared_ptr vào một biến cụ thể để duy trì lifetime cho đối tượng Account.
    const auto& hist_ref = std::make_shared<SavingsAccount>(77, "Tmp", 600.0)->getTransactionHistory();

    EXPECT_EQ(hist_ref.size(), 1u); // <- UB: hist_ref là tham chiếu lơ lửng
}

// [Trung bình] Con trỏ tới phần tử vector bị vô hiệu hóa do Reallocation (giống G02-Mid).
TEST_F(BankSystemTest, TUAF_Mid) {
    auto alice = sa(sid1);
    const auto& hist_ref = alice->getTransactionHistory();
    const Transaction* first_entry = &hist_ref[0]; 

    // Rootcause: Tester cache địa chỉ phần tử đầu tiên, sau đó thực hiện 57 thao tác làm vector reallocate.
    // Fix: Truy cập giá trị trực tiếp qua hist_ref[0] thay vì dùng con trỏ lưu địa chỉ cũ.
    for (int i = 0; i < 30; ++i) alice->deposit(10.0);
    // ... thực hiện thêm nhiều thao tác nạp/rút ...

    EXPECT_EQ(first_entry->description, "Initial deposit"); // <- UB: Con trỏ đã chết
}

// [Khó] Xóa phần tử ngay trong vòng lặp Range-based for (Iterator Invalidation).
TEST_F(BankSystemTest, TUAF_Hard) {
    std::vector<std::shared_ptr<Account>> candidates = bank.getAllAccounts();

    // Rootcause: Việc gọi candidates.erase() bên trong vòng lặp làm hỏng iterator nội bộ của range-for.
    // Fix: Sử dụng Erase-remove idiom hoặc duyệt bằng index ngược/iterator chuẩn và cập nhật sau khi xóa.
    for (const auto& acct : candidates) {
        if (acct->getBalance() < 100.0) {
            candidates.erase(std::find(candidates.begin(), candidates.end(), acct)); // <- UB
        }
    }
}

// ============================================================================
// G10. MEMORY / INDEX OVERRUN BY TESTER (Lỗi tràn mảng do tester viết sai)
// ============================================================================

// [Dễ] Truy cập index [0] khi chưa kiểm tra vector trả về có rỗng hay không.
TEST_F(BankSystemTest, TOverrun_Easy) {
    auto ghost_accts = bank.getAccountsByOwner("Nonexistent");

    // Rootcause: Tester mặc định luôn tìm thấy kết quả. Khi tìm chủ sở hữu không tồn tại, vector rỗng dẫn đến truy cập OOB.
    // Fix: Sử dụng ASSERT_FALSE(ghost_accts.empty()) trước khi truy cập phần tử đầu tiên.
    EXPECT_EQ(ghost_accts[0]->getOwnerName(), "Nonexistent"); 
}

// [Trung bình] Lỗi Off-by-one (Duyệt quá giới hạn mảng) trong vòng lặp kiểm tra.
TEST_F(BankSystemTest, TOverrun_Mid) {
    const auto& hist = alice->getTransactionHistory();

    // Rootcause: Điều kiện `<=` khiến vòng lặp cố gắng truy cập phần tử tại index bằng đúng size của vector.
    // Fix: Đổi `<=` thành `<` để đảm bảo index luôn nằm trong khoảng hợp lệ [0, size-1].
    for (size_t i = 0; i <= hist.size(); ++i) { 
        EXPECT_GT(hist[i].amount, 0.0);
    }
}

// [Khó] Duyệt theo size của live vector nhưng lại truy cập vào bản copy snapshot cũ (OOB).
TEST_F(BankSystemTest, TOverrun_Hard) {
    auto snapshot = alice->getTransactionHistory(); // Size N
    // ... thực hiện thêm 5 thao tác khiến live history tăng lên N+5 ...

    // Rootcause: Vòng lặp chạy live_hist.size() lần nhưng lại indexing vào snapshot chỉ có N phần tử.
    // Fix: Tách biệt logic xử lý snapshot cũ và các phần tử mới phát sinh trong live vector.
    for (size_t i = 0; i < live_hist.size(); ++i) {
        if (snapshot[i].amount != live_hist[i].amount) { // <- OOB khi i >= N
            // ...
        }
    }
}

// ============================================================================
// G11. WRONG ASSERTION VALUE (Sai lệch giá trị kỳ vọng do tester tính sai)
// ============================================================================

// [Dễ] Quên trừ lệ phí thấu chi (Overdraft fee) vào số dư kỳ vọng.
TEST_F(BankSystemTest, TAssert_Easy) {
    auto carol = ca(cid1); // balance 500, fee 20
    carol->withdraw(600.0);

    // Rootcause: Tester quên rằng CheckingAccount sẽ trừ thêm phí khi số dư âm.
    // Fix: Giá trị đúng phải là: Số dư ban đầu - Số tiền rút - Lệ phí thấu chi = -120.0.
    EXPECT_DOUBLE_EQ(carol->getBalance(), -100.0); // FAILS (Thực tế là -120)
}

// [Trung bình] Copy-paste lỗi: Kiểm tra nhầm ID tài khoản nguồn thay vì tài khoản đích.
TEST_F(BankSystemTest, TAssert_Mid) {
    alice->transfer(700.0, *bob);

    // Rootcause: Tester vô tình kiểm tra số dư của Alice (sid1) hai lần thay vì kiểm tra Bob (sid2).
    // Fix: Thay sid1 thành sid2 trong lệnh EXPECT kiểm tra số dư 3100.0.
    EXPECT_DOUBLE_EQ(bank.findAccount(sid1)->getBalance(), 600.0); 
    EXPECT_DOUBLE_EQ(bank.findAccount(sid1)->getBalance(), 3100.0); // FAILS (Vì vẫn check sid1)
}

// [Khó] Áp dụng sai công thức lãi đơn cho tài khoản tính lãi kép (Compound interest).
TEST_F(BankSystemTest, TAssert_Hard) {
    // Rootcause: Lãi kép thay đổi số gốc mỗi kỳ. Tester dùng công thức lãi đơn (Gốc * Rate * Kỳ) khiến kết quả lệch.
    // Fix: Sử dụng công thức lãi kép: Principal * ((1 + rate)^n) để tính số dư cuối cùng.
    double expected_final = principal + principal * bob->getInterestRate() * 4;
    EXPECT_DOUBLE_EQ(bob->getBalance(), expected_final); // FAILS (Lệch do lãi kép)
}

// ============================================================================
// G12. TIMEOUT / INFINITE LOOP (Vòng lặp vô tận khiến test bị treo)
// ============================================================================

// [Dễ] Thao tác "nạp" bên trong vòng lặp "rút" làm điều kiện dừng không bao giờ đạt tới.
TEST_F(BankSystemTest, TTimeout_Easy) {
    // Rootcause: Tester vừa rút 10 vừa nạp 1. Lượng rút không đủ làm balance giảm xuống dưới ngưỡng dừng.
    // Fix: Xóa lệnh deposit hoặc đảm bảo lượng rút lớn hơn hẳn lượng nạp trong mỗi vòng lặp.
    while (alice->withdraw(10.0)) {
        alice->deposit(1.0); 
    }
}

// [Trung bình] Thuật toán độ phức tạp O(N²) gây treo khi chạy lượng data lớn.
TEST_F(BankSystemTest, TTimeout_Mid) {
    // Rootcause: Gọi hàm getAccountsByOwner (O(N)) bên trong vòng lặp duyệt N tài khoản -> O(N^2).
    // Fix: Sử dụng map hoặc lưu trữ danh sách chủ sở hữu riêng để truy vấn trong O(1).
    for (int id : opened_ids) {
        auto same_owner = bank.getAccountsByOwner(acct->getOwnerName()); // <- O(N) scan
    }
}

// [Khó] Thay đổi số dư dương ròng (+net) trong mỗi vòng lặp khiến điều kiện dừng bị vượt qua.
TEST_F(BankSystemTest, TTimeout_Hard) {
    // Rootcause: Rút 0.01 nhưng nạp 0.5. Số dư tăng dần thay vì giảm về ngưỡng min -> Vòng lặp vô hạn.
    // Fix: Đảm bảo tổng thay đổi số dư trong mỗi lần lặp là số âm để tiến về ngưỡng dừng.
    for (int i = 0; i <= steps; ++i) {
        alice->withdraw(step_amount);
        alice->deposit(0.5); // Lỗi: Balance tăng dần
    }
}

// ============================================================================
// G13. VARIABLE SHADOWING (Lỗi che khuất biến/Khai báo trùng tên)
// ============================================================================

// [Dễ] Biến cục bộ trùng tên che khuất biến của Fixture (Fixture Member).
TEST_F(BankSystemTest, TShadow_Easy) {
    // Rootcause: Khai báo lại `int sid1` làm che khuất sid1 của Alice trong class cha. 
    // Mọi lệnh bank.findAccount(sid1) sau đó sẽ dùng tài khoản "ShadowUser" (ID mới).
    // Fix: Đổi tên biến cục bộ (ví dụ: new_sid) để tránh xung đột tên với Fixture.
    int sid1 = bank.openSavingsAccount("ShadowUser", 500.0, 0.03, 50.0);

    auto acct = bank.findAccount(sid1); // Tìm ra ShadowUser thay vì Alice
    EXPECT_EQ(acct->getOwnerName(), "Alice"); // FAILS: "ShadowUser"
}

// [Trung bình] Biến vòng lặp ghi đè lên biến lưu trữ ID bên ngoài.
TEST_F(BankSystemTest, TShadow_Mid) {
    int id = sid1; // id = 1 (Alice)

    // Rootcause: Vòng lặp tái sử dụng tên biến `id`, khiến giá trị ID của Alice bị ghi đè bởi ID của tài khoản BatchUser cuối cùng.
    // Fix: Sử dụng một tên biến khác cho vòng lặp (ví dụ: batch_id).
    for (int i = 0; i < 50; ++i) {
        id = bank.openSavingsAccount("BatchUser" + std::to_string(i), 200.0);
    }

    // LỖI: id lúc này không còn là 1 nữa mà trỏ tới tài khoản BatchUser cuối cùng.
    bank.findAccount(id)->deposit(500.0);
    EXPECT_EQ(bank.findAccount(id)->getOwnerName(), "Alice"); // FAILS
}

// [Khó] Trộn lẫn giữa biến cục bộ và biến Fixture sau khi đóng/mở tài khoản mới.
TEST_F(BankSystemTest, TShadow_Hard) {
    bank.closeAccount(sid1); // Đóng tài khoản Alice (ID 1)

    // Rootcause: Khai báo `int sid1` cục bộ (ví dụ ID 5) nhưng ở cuối hàm lại gọi hardcode `findAccount(1)`. 
    // Việc dùng trùng tên `sid1` gây nhầm lẫn cực lớn giữa "ID mới" và "ID cũ đã đóng".
    // Fix: Không bao giờ khai báo trùng tên với Fixture; tránh dùng số hardcode (1) mà hãy dùng biến rõ ràng.
    int sid1 = bank.openSavingsAccount("NewAlice", 1500.0);

    // LỖI: Kiểm tra tài khoản ID 1 (đã đóng) thay vì tài khoản NewAlice (ID 5).
    EXPECT_TRUE(bank.findAccount(1)->isActive()); // FAILS: Tài khoản 1 đã CLOSED
}

// ============================================================================
// G14. FALSE POSITIVE / VACUOUS TEST (Test luôn vượt qua nhưng không có giá trị)
// ============================================================================

// [Dễ] Các Assert quá lỏng lẻo, không kiểm tra chính xác giá trị thay đổi.
TEST_F(BankSystemTest, TFalsePos_Easy) {
    auto alice = bank.findAccount(sid1);
    double amount = 250.0;
    alice->deposit(amount);

    // Rootcause: EXPECT_GT và EXPECT_NE chỉ kiểm tra "có thay đổi" hoặc "là số dương", 
    // không chứng minh được nạp đúng 250.0 (nạp 1 cent cũng pass).
    // Fix: Sử dụng EXPECT_DOUBLE_EQ(balance_after, balance_before + amount);
    EXPECT_GT(alice->getBalance(), 0.0); // Vacuous: Luôn đúng với mọi số dương
    EXPECT_NE(alice->getBalance(), balance_before); // Không kiểm tra được số tiền cụ thể
}

// [Trung bình] Kiểm tra tổng tài sản (Total Assets) sau khi chuyển khoản.
TEST_F(BankSystemTest, TFalsePos_Mid) {
    // Rootcause: Giao dịch chuyển khoản là "Zero-sum" (tổng không đổi). Assert tổng tài sản luôn đúng 
    // ngay cả khi logic chuyển tiền sai (ví dụ: trừ sai số tiền nhưng cộng đúng tổng).
    // Fix: Assert số dư của TỪNG tài khoản riêng biệt tham gia giao dịch chuyển tiền.
    alice->transfer(400.0, *bob);
    EXPECT_DOUBLE_EQ(bank.getTotalAssets(), 5400.0); // Vô nghĩa: Chuyển kiểu gì tổng cũng vậy
}

// [Khó] Thiết kế Test Case làm triệt tiêu điều kiện cần kiểm tra (Overdraft).
TEST_F(BankSystemTest, TFalsePos_Hard) {
    // Rootcause: Tester nạp tiền (deposit) ngay trước khi rút (withdraw) trong vòng lặp. 
    // Điều này đảm bảo số dư luôn đủ, khiến logic thấu chi (Overdraft) không bao giờ thực sự chạy.
    // Fix: Rút số tiền vượt ngưỡng thấu chi mà KHÔNG nạp thêm tiền trước đó.
    for (int i = 1; i <= 10; ++i) {
        acct->deposit(withdraw_amount + 50.0); // LỖI: Luôn đủ tiền, không bao giờ thấu chi
        acct->withdraw(withdraw_amount);
    }
    EXPECT_EQ(acct->getOverdraftCount(), 0); // Pass một cách vô nghĩa vì thấu chi chưa từng xảy ra
}