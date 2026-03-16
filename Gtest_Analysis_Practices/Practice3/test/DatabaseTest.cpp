/**
 * DatabaseTest.cpp
 *
 * GTest suite for Database.cpp — designed as a TRAINEE EVALUATION exercise.
 *
 * Each test group targets a specific class of bug intentionally planted in the
 * test logic (NOT in Database.cpp itself). Trainees must:
 *   1. Read the test code carefully (dry-run / code-reading).
 *   2. Identify the root cause of the injected defect.
 *   3. Propose a correct fix.
 *
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │  GROUP │ BUG TYPE                     │ TEST NAMES                  │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │  G1    │ Segmentation Fault           │ SegFault_Easy/Mid/Hard      │
 * │  G2    │ Use After Free               │ UseAfterFree_Easy/Mid/Hard  │
 * │  G3    │ Memory / Index Overrun       │ Overrun_Easy/Mid/Hard       │
 * │  G4    │ Data Corruption              │ DataCorrupt_Easy/Mid/Hard   │
 * │  G5    │ Timeout / Infinite Loop      │ Timeout_Easy/Mid/Hard       │
 * │  G6    │ Double Free / Resource Leak  │ ResourceLeak_Easy/Mid/Hard  │
 * │  G7    │ Logic Bug (Subtle)           │ LogicBug_Easy/Mid/Hard      │
 * └─────────────────────────────────────────────────────────────────────┘
 *
 * Difficulty scale:
 *   Easy   — bug is immediately visible on first read.
 *   Medium — requires tracing data flow across multiple steps; setup is
 *            intentionally verbose to distract from the real defect.
 *   Hard   — test appears completely correct and well-structured; the defect
 *            is contextual and only visible after deep analysis of internal
 *            state or container semantics.
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <set>

#include "Database.hpp"
#include "Book.hpp"
#include "User.hpp"
#include "Transaction.hpp"

// ============================================================
// Shared fixture — gives each test a fresh, populated Database
// ============================================================
class DatabaseTest : public ::testing::Test {
protected:
    Database db;

    int book_id1  = -1;
    int book_id2  = -1;
    int book_id3  = -1;
    int user_id1  = -1;
    int user_id2  = -1;
    int user_id3  = -1;
    int tx_id1    = -1;
    int tx_id2    = -1;

    void SetUp() override {
        book_id1 = db.createBook("Clean Code","Robert Martin","978-0132350884",2008);
        book_id2 = db.createBook("The Pragmatic Programmer","David Thomas","978-0201616224",1999);
        book_id3 = db.createBook("Design Patterns","Gang of Four","978-0201633610",1994);

        user_id1 = db.createUser("alice","Alice","Smith","alice@example.com","0901000001","pass1");
        user_id2 = db.createUser("bob","Bob","Jones","bob@example.com","0901000002","pass2");
        user_id3 = db.createUser("charlie","Charlie","Brown","charlie@example.com","0901000003","pass3");

        auto b1 = db.readBook(book_id1);
        auto u1 = db.readUser(user_id1);
        auto b2 = db.readBook(book_id2);
        auto u2 = db.readUser(user_id2);

        tx_id1 = db.createTransaction("borrow", b1, u1);
        tx_id2 = db.createTransaction("borrow", b2, u2);
    }
};

// ============================================================
// GROUP 1 — Segmentation Fault (null / dangling pointer)
// ============================================================

/**
 * G1-Easy | Segmentation Fault
 */
TEST_F(DatabaseTest, SegFault_Easy) {
    auto book_ptr = db.readBook(book_id1);
    Book* raw = book_ptr.get();
    book_ptr.reset();
    EXPECT_EQ(raw->getTitle(), "Clean Code");
}

/**
 * G1-Medium | Segmentation Fault
 */
TEST_F(DatabaseTest, SegFault_Mid) {
    auto tx1 = db.readTransaction(tx_id1);
    auto tx2 = db.readTransaction(tx_id2);
    ASSERT_NE(tx1, nullptr);
    ASSERT_NE(tx2, nullptr);

    db.updateTransaction(tx_id1,"status","completed");
    db.updateTransaction(tx_id2,"status","completed");

    int new_user_id = db.createUser("diana","Diana","Prince","diana@example.com","0901000004","pass4");
    int new_book_id = db.createBook("Refactoring","Martin Fowler","978-0201485677",1999);

    auto new_book = db.readBook(new_book_id);
    auto new_user = db.readUser(new_user_id);
    int new_tx_id = db.createTransaction("borrow", new_book, new_user);
    db.updateTransaction(new_tx_id,"status","open");

    auto user_txns = db.queryTransactionsByUserID(new_user_id);
    ASSERT_FALSE(user_txns.empty());

    for (const auto& tx : user_txns) {
        EXPECT_EQ(tx->getUser()->getID(), new_user_id);
        EXPECT_EQ(tx->getBook()->getID(), new_book_id);

        std::shared_ptr<Book> extra_book = nullptr;
        try {
            extra_book = db.readBook(new_book_id + 99);
        } catch (const std::invalid_argument&) {}

        EXPECT_EQ(extra_book->getTitle(), "Refactoring");
    }
}

/**
 * G1-Hard | Segmentation Fault
 */
TEST_F(DatabaseTest, SegFault_Hard) {
    auto b3 = db.readBook(book_id3);
    auto u1 = db.readUser(user_id1);
    auto u3 = db.readUser(user_id3);

    int tx3 = db.createTransaction("borrow", b3, u1);
    int tx4 = db.createTransaction("borrow", db.readBook(book_id1), u3);

    db.readTransaction(tx_id1)->execute();
    db.readTransaction(tx_id2)->execute();
    db.readTransaction(tx3)->execute();
    db.readTransaction(tx4)->execute();

    db.updateTransaction(tx_id1,"status","completed");
    db.updateTransaction(tx_id2,"status","completed");

    auto u1_txns = db.queryTransactionsByUserID(user_id1);
    auto u2_txns = db.queryTransactionsByUserID(user_id2);
    auto u3_txns = db.queryTransactionsByUserID(user_id3);

    ASSERT_FALSE(u1_txns.empty());
    ASSERT_FALSE(u2_txns.empty());
    ASSERT_FALSE(u3_txns.empty());

    db.deleteBook(book_id1);

    EXPECT_THROW(db.readBook(book_id1), std::invalid_argument);
    EXPECT_NO_THROW(db.readBook(book_id2));
    EXPECT_NO_THROW(db.readBook(book_id3));

    auto dangling_txns = db.queryTransactionsByBookID(book_id1);
    EXPECT_TRUE(dangling_txns.empty());
}

/**
 * G2-Easy | Use After Free
 */
TEST_F(DatabaseTest, UseAfterFree_Easy) {
    auto user = db.readUser(user_id1);
    db.deleteUser(user_id1);
    EXPECT_NO_THROW(db.updateUser(user_id1,"email","newemail@example.com"));
}

/**
 * G2-Medium | Use After Free
 */
TEST_F(DatabaseTest, UseAfterFree_Mid) {
    auto borrow_tx = db.readTransaction(tx_id1);
    borrow_tx->execute();
    EXPECT_EQ(borrow_tx->getStatus(),"completed");

    auto book1_before = db.readBook(book_id1);
    EXPECT_FALSE(book1_before->isAvailable());

    db.deleteBook(book_id1);

    auto orphaned_book = borrow_tx->getBook();
    ASSERT_NE(orphaned_book,nullptr);

    int return_tx_id = db.createTransaction("return", orphaned_book, db.readUser(user_id1));
    auto return_tx = db.readTransaction(return_tx_id);
    return_tx->execute();

    orphaned_book->setIsAvailable(true);
    EXPECT_TRUE(orphaned_book->isAvailable());

    EXPECT_NO_THROW({
        auto db_book = db.readBook(book_id1);
        EXPECT_TRUE(db_book->isAvailable());
    });
}

/**
 * G2-Hard | Use After Free
 */
TEST_F(DatabaseTest, UseAfterFree_Hard) {
    const auto& books_snapshot = db.getBooks();
    ASSERT_GE(books_snapshot.size(),1u);

    const Book* first_book_raw = books_snapshot.front().get();
    std::string expected_title = books_snapshot.front()->getTitle();
    size_t size_before = books_snapshot.size();

    for (int i=0;i<128;++i) {
        db.createBook(
            "Extra Book "+std::to_string(i),
            "Extra Author"+std::to_string(i),
            "EAN-"+std::to_string(1000+i),
            2000+(i%24)
        );
    }

    size_t size_after = books_snapshot.size();
    EXPECT_EQ(size_after, size_before+128);
    EXPECT_EQ(books_snapshot[0]->getTitle(), expected_title);
    EXPECT_EQ(books_snapshot[0].get(), first_book_raw);
}

/**
 * G3-Easy | Index Overrun
 */
TEST_F(DatabaseTest, Overrun_Easy) {
    const auto& books = db.getBooks();
    std::string last_title;

    for (size_t i=0;i<=books.size();++i) {
        last_title = books[i]->getTitle();
    }

    EXPECT_FALSE(last_title.empty());
}

/**
 * G3-Medium | Index Overrun
 */
TEST_F(DatabaseTest, Overrun_Mid) {

    int b4 = db.createBook("The Clean Coder","Robert Martin","978-0137081073",2011);
    int b5 = db.createBook("Clean Architecture","Robert Martin","978-0134494166",2017);

    auto new_book = db.readBook(b4);
    auto u2 = db.readUser(user_id2);

    int borrow_tx = db.createTransaction("borrow", new_book, u2);
    db.readTransaction(borrow_tx)->execute();
    db.updateTransaction(borrow_tx,"status","completed");

    auto u2_txns = db.queryTransactionsByUserID(user_id2);
    ASSERT_FALSE(u2_txns.empty());

    auto results = db.queryBooks("Robert Martin",2);
    ASSERT_GE(results.size(),1u);

    std::vector<std::string> title_summary;

    for (size_t i=0;i<results.size();++i) {
        title_summary.push_back(results[i]->getTitle());
        EXPECT_FALSE(results[i]->getTitle().empty());
    }

    std::string guard_title = results[results.size()]->getTitle();
    EXPECT_TRUE(guard_title.empty());
}

/**
 * G3-Hard | Memory Overrun
 */
TEST_F(DatabaseTest, Overrun_Hard) {

    auto initial = db.queryBooks("Robert Martin",0);
    ASSERT_EQ(initial.size(),1u);

    std::vector<std::string> name_variants = {
        "R. Martin","Robert C. Martin","Bob Martin",
        "R.C. Martin","Robert C Martin","Uncle Bob",
        "Robert Martin Jr","R Martin","Bob C. Martin",
        "Robert Martin"
    };

    for (const auto& name : name_variants) {
        db.updateBook(book_id1,"author",name);
    }

    db.updateBook(book_id2,"author","Dave Thomas");
    db.updateBook(book_id2,"author","D. Thomas");
    db.updateBook(book_id2,"author","David Thomas");

    db.updateBook(book_id3,"author","GoF");
    db.updateBook(book_id3,"author","Gamma et al.");
    db.updateBook(book_id3,"author","Gang of Four");

    auto final_rm = db.queryBooks("Robert Martin",0);
    auto final_dt = db.queryBooks("David Thomas",0);
    auto final_gf = db.queryBooks("Gang of Four",0);

    EXPECT_EQ(final_rm.size(),1u);
    EXPECT_EQ(final_dt.size(),1u);
    EXPECT_EQ(final_gf.size(),1u);

    auto ghost_uncle_bob = db.queryBooks("Uncle Bob",0);
    auto ghost_r_martin  = db.queryBooks("R. Martin",0);
    auto ghost_gof       = db.queryBooks("GoF",0);

    EXPECT_TRUE(ghost_uncle_bob.empty());
    EXPECT_TRUE(ghost_r_martin.empty());
    EXPECT_TRUE(ghost_gof.empty());
}

// ============================================================
// GROUP 4 — Data Corruption
// ============================================================

/**
 * G4-Easy | Data Corruption
 */
TEST_F(DatabaseTest, DataCorrupt_Easy) {
    // BUG INJECTED: wrong field name casing — "Available" vs "available"
    db.updateBook(book_id1, "Available", "false");

    auto book = db.readBook(book_id1);
    EXPECT_FALSE(book->isAvailable());  // FAILS — update was silently a no-op
}


/**
 * G4-Medium | Data Corruption
 */
TEST_F(DatabaseTest, DataCorrupt_Mid) {
    auto book1 = db.readBook(book_id1);
    auto book2 = db.readBook(book_id2);
    auto book3 = db.readBook(book_id3);
    auto u1    = db.readUser(user_id1);
    auto u2    = db.readUser(user_id2);
    auto u3    = db.readUser(user_id3);

    db.readTransaction(tx_id1)->execute();
    db.readTransaction(tx_id2)->execute();

    int tx3 = db.createTransaction("borrow", book3, u3);
    db.readTransaction(tx3)->execute();
    db.updateTransaction(tx3, "status", "completed");

    EXPECT_FALSE(book1->isAvailable());
    EXPECT_FALSE(book2->isAvailable());
    EXPECT_FALSE(book3->isAvailable());

    int return_tx1 = db.createTransaction("return", book2, u1);  // <- wrong book
    db.readTransaction(return_tx1)->execute();
    db.updateTransaction(return_tx1, "status", "completed");

    int return_tx2 = db.createTransaction("return", book2, u2);
    db.readTransaction(return_tx2)->execute();
    db.updateTransaction(return_tx2, "status", "completed");

    int return_tx3 = db.createTransaction("return", book3, u3);
    db.readTransaction(return_tx3)->execute();
    db.updateTransaction(return_tx3, "status", "completed");

    EXPECT_TRUE(book1->isAvailable());
    EXPECT_TRUE(book2->isAvailable());
    EXPECT_TRUE(book3->isAvailable());
}


/**
 * G4-Hard | Data Corruption
 */
TEST_F(DatabaseTest, DataCorrupt_Hard) {
    db.updateUser(user_id3, "username", "charlie_renamed");
    auto u3_check = db.authenticateUser("charlie_renamed", "pass3");
    EXPECT_EQ(u3_check->getID(), user_id3);

    int new_uid = db.createUser("newuser", "New", "User",
                                "new@example.com", "0901000099", "passNew");
    auto new_u = db.authenticateUser("newuser", "passNew");
    EXPECT_EQ(new_u->getID(), new_uid);

    db.updateUser(user_id1, "email", "alice_new@example.com");
    db.updateUser(user_id1, "phone", "0909999999");
    auto u1_fresh = db.authenticateUser("alice", "pass1");
    EXPECT_EQ(u1_fresh->getEmail(), "alice_new@example.com");

    db.updateUser(user_id2, "username", "alice");

    auto authenticated = db.authenticateUser("alice", "pass1");

    EXPECT_EQ(authenticated->getID(), user_id1);
    EXPECT_EQ(authenticated->getEmail(), "alice_new@example.com");
}


// ============================================================
// GROUP 5 — Timeout / Infinite Loop
// ============================================================

/**
 * G5-Easy | Infinite Loop
 */
TEST_F(DatabaseTest, Timeout_Easy) {
    const auto& books = db.getBooks();
    size_t i = 0;
    std::string found_title;

    while (i < books.size()) {
        if (books[i]->getTitle() == "Nonexistent") {
            ++i;  
        }
        found_title = books[i]->getTitle();
    }

    EXPECT_FALSE(found_title.empty());
}


/**
 * G5-Medium | Timeout
 */
TEST_F(DatabaseTest, Timeout_Mid) {
    const int N = 400;

    std::vector<int> created_ids;
    created_ids.reserve(N);
    for (int i = 0; i < N; ++i) {
        int id = db.createBook(
            "Library Book "  + std::to_string(i),
            "Staff Author "  + std::to_string(i % 20),
            "ISBN-LIB-"      + std::to_string(10000 + i),
            1900 + (i % 125)
        );
        created_ids.push_back(id);
    }

    ASSERT_EQ(static_cast<int>(db.getBooks().size()), N + 3);

    auto u1 = db.readUser(user_id1);
    auto u2 = db.readUser(user_id2);
    int sample_tx1 = db.createTransaction("borrow", db.readBook(created_ids[0]), u1);
    int sample_tx2 = db.createTransaction("borrow", db.readBook(created_ids[1]), u2);
    db.readTransaction(sample_tx1)->execute();
    db.readTransaction(sample_tx2)->execute();

    int found_count = 0;
    const auto& all_books = db.getBooks();
    for (size_t i = 0; i < all_books.size(); ++i) {
        auto results = db.queryBooks(all_books[i]->getTitle(), 100);
        if (!results.empty()) ++found_count;
    }

    EXPECT_EQ(found_count, static_cast<int>(all_books.size()));
}


/**
 * G5-Hard | Timeout
 */
TEST_F(DatabaseTest, Timeout_Hard) {

    int total_before = static_cast<int>(db.getTransactions().size());

    auto b3 = db.readBook(book_id3);
    auto u3 = db.readUser(user_id3);
    int tx3 = db.createTransaction("borrow", b3, u3);

    auto u1 = db.readUser(user_id1);
    int tx4 = db.createTransaction("return", db.readBook(book_id2), u1);

    db.readTransaction(tx_id1)->execute();
    db.readTransaction(tx_id2)->execute();
    db.updateTransaction(tx3, "status", "open");
    db.updateTransaction(tx4, "status", "open");

    int cancelled_count = 0;
    std::vector<int> all_user_ids = {user_id1, user_id2, user_id3};
    for (int uid : all_user_ids) {
        std::vector<std::shared_ptr<Transaction>> live_txns =
            db.queryTransactionsByUserID(uid);

        for (const auto& tx : live_txns) {
            db.deleteTransaction(tx->getID());
            ++cancelled_count;
        }
    }

    EXPECT_TRUE(db.getTransactions().empty());
    EXPECT_EQ(cancelled_count, total_before);
}


// ============================================================
// GROUP 6 — Double Free / Resource Leak
// ============================================================

/**
 * G6-Easy | Double Delete
 */
TEST_F(DatabaseTest, ResourceLeak_Easy) {

    db.deleteBook(book_id1);
    db.deleteBook(book_id1);

    EXPECT_THROW(db.readBook(book_id1), std::invalid_argument);
}


/**
 * G6-Medium | Resource Leak
 */
TEST_F(DatabaseTest, ResourceLeak_Mid) {

    auto u1_txns_before = db.queryTransactionsByUserID(user_id1);
    ASSERT_FALSE(u1_txns_before.empty());

    for (const auto& tx : u1_txns_before) {
        db.updateTransaction(tx->getID(), "status", "cancelled");
    }

    for (const auto& tx : u1_txns_before) {
        EXPECT_EQ(tx->getStatus(), "cancelled");
    }

    auto b3 = db.readBook(book_id3);
    auto u2 = db.readUser(user_id2);
    int tx_new = db.createTransaction("borrow", b3, u2);
    db.readTransaction(tx_new)->execute();
    EXPECT_FALSE(db.queryTransactionsByUserID(user_id2).empty());

    size_t users_before = db.getUsers().size();

    db.deleteUser(user_id1);

    EXPECT_EQ(db.getUsers().size(), users_before - 1);
    EXPECT_THROW(db.authenticateUser("alice", "pass1"), std::invalid_argument);

    EXPECT_FALSE(db.queryTransactionsByUserID(user_id2).empty());

    auto u1_txns_after = db.queryTransactionsByUserID(user_id1);
    EXPECT_TRUE(u1_txns_after.empty());
}


/**
 * G6-Hard | Resource Leak
 */
TEST_F(DatabaseTest, ResourceLeak_Hard) {

    int b4 = db.createBook("Working Effectively with Legacy Code",
                            "Michael Feathers", "978-0131177055", 2004);

    int u4 = db.createUser("eve", "Eve", "Adams",
                            "eve@example.com", "0901000005", "pass5");

    auto eb = db.readBook(b4);
    auto eu = db.readUser(u4);

    int tx_extra = db.createTransaction("borrow", eb, eu);
    db.readTransaction(tx_extra)->execute();

    size_t original_book_count = db.getBooks().size();
    size_t original_user_count = db.getUsers().size();
    size_t original_tx_count   = db.getTransactions().size();

    Database db_copy = db;

    db_copy.createBook("Ghost Book", "Ghost Author", "GHT-000", 2025);

    EXPECT_EQ(db.getBooks().size(), original_book_count);
    EXPECT_EQ(db_copy.getBooks().size(), original_book_count + 1);

    db_copy.deleteUser(u4);

    EXPECT_EQ(db.getUsers().size(), original_user_count);
    EXPECT_EQ(db_copy.getUsers().size(), original_user_count - 1);

    EXPECT_EQ(db.getTransactions().size(), original_tx_count);
}


// ============================================================
// GROUP 7 — Logic Bug
// ============================================================

/**
 * G7-Easy | Logic Bug
 */
TEST_F(DatabaseTest, LogicBug_Easy) {

    EXPECT_NO_THROW({
        auto user = db.authenticateUser("ALICE", "PASS1");
        EXPECT_EQ(user->getUsername(), "alice");
    });
}


/**
 * G7-Medium | Logic Bug
 */
TEST_F(DatabaseTest, LogicBug_Mid) {

    db.createBook("Clean Architecture", "Robert Martin", "978-0134494166", 2017);
    db.createBook("Code Complete", "Steve McConnell","978-0735619678", 2004);
    db.createBook("The Mythical Man-Month","Fred Brooks","978-0201835953", 1995);

    auto u3 = db.readUser(user_id3);
    auto b3 = db.readBook(book_id3);

    int tx = db.createTransaction("borrow", b3, u3);
    db.readTransaction(tx)->execute();

    auto r1 = db.queryBooks("Clen Code", 2);
    EXPECT_FALSE(r1.empty());

    auto r2 = db.queryBooks("Cod Complet", 2);
    EXPECT_FALSE(r2.empty());

    auto r3 = db.queryBooks("Pragmatik Programmr", 2);
    EXPECT_FALSE(r3.empty());
}

/**
 * G7-Hard | Logic Bug
 */
TEST_F(DatabaseTest, LogicBug_Hard) {

    int b4 = db.createBook("Clean Architecture","Robert Martin","978-0134494166",2017);
    int b5 = db.createBook("Patterns of Enterprise","Martin Fowler","978-0321127426",2002);
    int b6 = db.createBook("Refactoring","Martin Fowler","978-0201485677",1999);

    auto u1 = db.readUser(user_id1);
    auto u2 = db.readUser(user_id2);

    int tx_b4 = db.createTransaction("borrow", db.readBook(b4), u1);
    int tx_b5 = db.createTransaction("borrow", db.readBook(b5), u2);

    db.readTransaction(tx_b4)->execute();
    db.readTransaction(tx_b5)->execute();

    db.updateTransaction(tx_b4, "status", "completed");
    db.updateTransaction(tx_b5, "status", "completed");

    db.updateBook(book_id1, "author", "Robert C. Martin");
    db.updateBook(b4, "author", "Robert C. Martin");
    db.updateBook(book_id2, "author", "Dave Thomas");

    std::vector<std::pair<std::string, int>> author_expected = {
        {"robert c. martin", 2},
        {"dave thomas", 1},
        {"gang of four", 1},
        {"martin fowler", 2}
    };

    int total_hits = 0;
    for (const auto& [author, expected_count] : author_expected) {
        auto results = db.queryBooks(author, 0);
        EXPECT_EQ(static_cast<int>(results.size()), expected_count);
        total_hits += static_cast<int>(results.size());
    }

    EXPECT_EQ(total_hits, 6);

    auto ghost_old_martin = db.queryBooks("robert martin", 0);
    auto ghost_old_thomas = db.queryBooks("david thomas", 0);

    EXPECT_TRUE(ghost_old_martin.empty());
    EXPECT_TRUE(ghost_old_thomas.empty());
}