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
 * │  GROUP │ BUG TYPE                     │ TEST NAMES                 │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │  G1    │ Segmentation Fault           │ SegFault_Easy/Mid/Hard     │
 * │  G2    │ Use After Free               │ UseAfterFree_Easy/Mid/Hard │
 * │  G3    │ Memory / Index Overrun       │ Overrun_Easy/Mid/Hard      │
 * │  G4    │ Data Corruption              │ DataCorrupt_Easy/Mid/Hard  │
 * │  G5    │ Timeout / Infinite Loop      │ Timeout_Easy/Mid/Hard      │
 * │  G6    │ Double Free / Resource Leak  │ ResourceLeak_Easy/Mid/Hard │
 * │  G7    │ Logic Bug (Subtle)           │ LogicBug_Easy/Mid/Hard     │
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
        book_id1 = db.createBook("Clean Code",                "Robert Martin", "978-0132350884", 2008);
        book_id2 = db.createBook("The Pragmatic Programmer",  "David Thomas",  "978-0201616224", 1999);
        book_id3 = db.createBook("Design Patterns",           "Gang of Four",  "978-0201633610", 1994);

        user_id1 = db.createUser("alice",   "Alice",   "Smith",   "alice@example.com",   "0901000001", "pass1");
        user_id2 = db.createUser("bob",     "Bob",     "Jones",   "bob@example.com",     "0901000002", "pass2");
        user_id3 = db.createUser("charlie", "Charlie", "Brown",   "charlie@example.com", "0901000003", "pass3");

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
 *
 * Hint: Pay attention to what happens to the pointer BEFORE it is used.
 *
 * BUG: A raw pointer `raw` is captured from a shared_ptr, then the shared_ptr
 *      is reset(), destroying the managed Book. Accessing `raw` afterwards is
 *      a classic dangling pointer dereference / segfault.
 */
TEST_F(DatabaseTest, SegFault_Easy) {
    auto book_ptr = db.readBook(book_id1);
    Book* raw = book_ptr.get();

    // BUG INJECTED: resetting the only local owner makes raw a dangling pointer
    book_ptr.reset();

    EXPECT_EQ(raw->getTitle(), "Clean Code");  // <- segfault here
}

/**
 * G1-Medium | Segmentation Fault
 *
 * Hint: Carefully trace the state of every pointer across all branches.
 *
 * BUG: The test performs a realistic multi-step workflow: it looks up books,
 *      creates a new user, issues a borrow transaction, then queries the user's
 *      transaction history to validate each record. All steps up to the inner
 *      loop are correct. Inside the loop, the code attempts to look up a book
 *      by a non-existent ID (new_book_id + 99); the exception is silently
 *      swallowed in a catch block, leaving `extra_book` as nullptr. The very
 *      next line dereferences it — segfault. The surrounding correct-looking
 *      workflow is pure misdirection.
 */
TEST_F(DatabaseTest, SegFault_Mid) {
    // Step 1: confirm both pre-existing transactions are retrievable
    auto tx1 = db.readTransaction(tx_id1);
    auto tx2 = db.readTransaction(tx_id2);
    ASSERT_NE(tx1, nullptr);
    ASSERT_NE(tx2, nullptr);

    // Step 2: mark both transactions as completed
    db.updateTransaction(tx_id1, "status", "completed");
    db.updateTransaction(tx_id2, "status", "completed");

    // Step 3: create a new user and a new book, issue a fresh borrow transaction
    int new_user_id = db.createUser("diana", "Diana", "Prince",
                                    "diana@example.com", "0901000004", "pass4");
    int new_book_id = db.createBook("Refactoring", "Martin Fowler",
                                    "978-0201485677", 1999);
    auto new_book = db.readBook(new_book_id);
    auto new_user = db.readUser(new_user_id);
    int new_tx_id = db.createTransaction("borrow", new_book, new_user);
    db.updateTransaction(new_tx_id, "status", "open");

    // Step 4: retrieve all transactions for the new user and validate each one
    auto user_txns = db.queryTransactionsByUserID(new_user_id);
    ASSERT_FALSE(user_txns.empty());

    for (const auto& tx : user_txns) {
        EXPECT_EQ(tx->getUser()->getID(), new_user_id);
        EXPECT_EQ(tx->getBook()->getID(), new_book_id);

        // BUG INJECTED: attempting to read a non-existent book ID;
        // exception is caught but `extra_book` remains nullptr
        std::shared_ptr<Book> extra_book = nullptr;
        try {
            extra_book = db.readBook(new_book_id + 99);  // does not exist — throws
        } catch (const std::invalid_argument&) {
            // silently swallowed — extra_book stays nullptr
        }

        // Trainee question: can `extra_book` ever be non-null here?
        EXPECT_EQ(extra_book->getTitle(), "Refactoring");  // <- null dereference
    }
}

/**
 * G1-Hard | Segmentation Fault
 *
 * Hint: Consider the lifetime of objects stored inside the transaction maps
 *       after a book deletion, and think about what the final assertion proves.
 *
 * BUG: The test simulates a complete library session — three books borrowed,
 *      transactions executed and updated, one book administratively removed.
 *      Deleting book_id1 removes it from the `books` vector and search maps,
 *      but transactions_by_book_id[book_id1] is never cleared; it still holds
 *      shared_ptrs to Transactions that in turn hold shared_ptrs to the deleted
 *      Book (keeping the Book object alive — no crash). The final assertion
 *      checks that queryTransactionsByBookID(book_id1) is EMPTY — which is
 *      WRONG; the map entry survives. All prior steps pass cleanly and act as
 *      misdirection. The real bug is the inverted assertion masking a
 *      stale-map corruption in the DB internals.
 */
TEST_F(DatabaseTest, SegFault_Hard) {
    // Step 1: borrow all three books across multiple users
    auto b3 = db.readBook(book_id3);
    auto u1 = db.readUser(user_id1);
    auto u3 = db.readUser(user_id3);
    int tx3 = db.createTransaction("borrow", b3, u1);
    int tx4 = db.createTransaction("borrow", db.readBook(book_id1), u3);

    // Step 2: execute all four transactions
    db.readTransaction(tx_id1)->execute();
    db.readTransaction(tx_id2)->execute();
    db.readTransaction(tx3)->execute();
    db.readTransaction(tx4)->execute();

    // Step 3: mark two transactions as completed
    db.updateTransaction(tx_id1, "status", "completed");
    db.updateTransaction(tx_id2, "status", "completed");

    // Step 4: verify each user's transaction history is non-empty
    auto u1_txns = db.queryTransactionsByUserID(user_id1);
    auto u2_txns = db.queryTransactionsByUserID(user_id2);
    auto u3_txns = db.queryTransactionsByUserID(user_id3);
    ASSERT_FALSE(u1_txns.empty());
    ASSERT_FALSE(u2_txns.empty());
    ASSERT_FALSE(u3_txns.empty());

    // Step 5: admin removes book1 from the catalogue
    db.deleteBook(book_id1);

    // Step 6: confirm book1 is no longer retrievable via readBook
    EXPECT_THROW(db.readBook(book_id1), std::invalid_argument);

    // Step 7: confirm remaining books are still accessible
    EXPECT_NO_THROW(db.readBook(book_id2));
    EXPECT_NO_THROW(db.readBook(book_id3));

    // Step 8: query transactions that referenced the deleted book
    auto dangling_txns = db.queryTransactionsByBookID(book_id1);

    // BUG INJECTED: assertion is inverted — transactions_by_book_id was NEVER
    // cleaned up by deleteBook, so dangling_txns is NOT empty.
    // The test incorrectly asserts empty, masking the stale-map corruption.
    EXPECT_TRUE(dangling_txns.empty());  // WRONG — should be EXPECT_FALSE
}


// ============================================================
// GROUP 2 — Use After Free
// ============================================================

/**
 * G2-Easy | Use After Free
 *
 * Hint: Look at when the object is erased relative to when it is used.
 *
 * BUG: deleteUser is called, then updateUser is called on the same ID.
 *      findByID inside updateUser throws because the user no longer exists.
 */
TEST_F(DatabaseTest, UseAfterFree_Easy) {
    auto user = db.readUser(user_id1);
    db.deleteUser(user_id1);

    // BUG INJECTED: user no longer in database — findByID throws
    EXPECT_NO_THROW(db.updateUser(user_id1, "email", "newemail@example.com"));
}

/**
 * G2-Medium | Use After Free
 *
 * Hint: Track object ownership carefully across deleteBook and subsequent
 *       mutations — shared_ptr keeps objects alive, but the DB loses track.
 *
 * BUG: The test builds a plausible borrow-and-return workflow. Mid-session,
 *      the admin calls deleteBook(book_id1). The Transaction object still
 *      holds a shared_ptr to the Book (keeping it alive), so all pointer
 *      accesses succeed without crashing. However, calling setIsAvailable(true)
 *      on the orphaned Book mutates an object the Database no longer tracks —
 *      the DB's internal state is now inconsistent with the object's state.
 *      The final EXPECT_NO_THROW wraps a readBook call that throws because
 *      book_id1 is gone from the DB — silently swallowed, masking the error.
 */
TEST_F(DatabaseTest, UseAfterFree_Mid) {
    // Step 1: execute the existing borrow transaction for book1 / user1
    auto borrow_tx = db.readTransaction(tx_id1);
    borrow_tx->execute();
    EXPECT_EQ(borrow_tx->getStatus(), "completed");

    // Step 2: confirm book1 is marked unavailable through the DB
    auto book1_before = db.readBook(book_id1);
    EXPECT_FALSE(book1_before->isAvailable());

    // Step 3: admin removes book1 from the catalogue mid-session
    // BUG INJECTED: book is orphaned from the DB here
    db.deleteBook(book_id1);

    // Step 4: the transaction still holds a shared_ptr to the deleted book
    auto orphaned_book = borrow_tx->getBook();
    ASSERT_NE(orphaned_book, nullptr);  // shared_ptr valid — no crash yet

    // Step 5: simulate user returning the book via the orphaned pointer
    int return_tx_id = db.createTransaction("return", orphaned_book,
                                             db.readUser(user_id1));
    auto return_tx = db.readTransaction(return_tx_id);
    return_tx->execute();

    // Step 6: mutate the orphaned object — invisible to the DB
    orphaned_book->setIsAvailable(true);
    EXPECT_TRUE(orphaned_book->isAvailable());  // object-level: true

    // Step 7: BUG INJECTED — readBook throws (book deleted), exception swallowed
    // Trainee question: why does this throw, and what is the actual DB state?
    EXPECT_NO_THROW({
        auto db_book = db.readBook(book_id1);  // <- throws std::invalid_argument
        EXPECT_TRUE(db_book->isAvailable());
    });
}

/**
 * G2-Hard | Use After Free (iterator invalidation via reference into vector)
 *
 * Hint: Think about what happens to a const-reference to an internal vector
 *       after the vector reallocates its buffer.
 *
 * BUG: `books_snapshot` is a const-reference to the Database's internal
 *      std::vector<shared_ptr<Book>>. After 128 createBook calls, the vector
 *      is very likely to have reallocated its buffer — making `books_snapshot`
 *      a dangling reference. Any subsequent access through `books_snapshot`
 *      (including `books_snapshot.size()` and `books_snapshot[0]`) is
 *      undefined behaviour. The managed Book objects themselves remain alive
 *      (shared_ptr refcounts > 0), so `first_book_raw` is still valid — which
 *      makes the final EXPECT_EQ coincidentally pass, masking the underlying
 *      UB on the dangling reference.
 */
TEST_F(DatabaseTest, UseAfterFree_Hard) {
    // Step 1: capture a const-reference to the internal books vector
    const auto& books_snapshot = db.getBooks();
    ASSERT_GE(books_snapshot.size(), 1u);

    // Step 2: record the raw managed pointer and title of the first book
    const Book* first_book_raw  = books_snapshot.front().get();
    std::string expected_title  = books_snapshot.front()->getTitle();
    size_t      size_before     = books_snapshot.size();

    // Step 3: add a large batch of books — very likely to trigger reallocation
    // BUG INJECTED: books_snapshot becomes a dangling reference after realloc
    for (int i = 0; i < 128; ++i) {
        db.createBook("Extra Book "  + std::to_string(i),
                      "Extra Author" + std::to_string(i),
                      "EAN-"         + std::to_string(1000 + i),
                      2000 + (i % 24));
    }

    // Step 4: access through the (potentially dangling) reference — UB
    size_t size_after = books_snapshot.size();       // <- UB if buffer reallocated
    EXPECT_EQ(size_after, size_before + 128);

    // Step 5: index into the dangling reference — UB
    // Trainee question: is books_snapshot[0] still valid here?
    EXPECT_EQ(books_snapshot[0]->getTitle(), expected_title);  // <- potential UB

    // Step 6: raw pointer comparison — may coincidentally pass due to allocator
    // behaviour, further masking the UB above
    EXPECT_EQ(books_snapshot[0].get(), first_book_raw);
}


// ============================================================
// GROUP 3 — Memory / Index Overrun
// ============================================================

/**
 * G3-Easy | Index Overrun
 *
 * Hint: Check the loop termination condition against container size.
 *
 * BUG: Loop condition uses `<=` instead of `<`, accessing one element past
 *      the end of the vector on the final iteration.
 */
TEST_F(DatabaseTest, Overrun_Easy) {
    const auto& books = db.getBooks();
    std::string last_title;

    // BUG INJECTED: `<= books.size()` causes OOB on final iteration
    for (size_t i = 0; i <= books.size(); ++i) {
        last_title = books[i]->getTitle();  // <- OOB when i == books.size()
    }

    EXPECT_FALSE(last_title.empty());
}

/**
 * G3-Medium | Index Overrun
 *
 * Hint: Trace the actual number of results returned before any direct indexing.
 *
 * BUG: The test constructs a plausible scenario — extra books are added,
 *      transactions are executed, a search is run, and results are summarised
 *      in a report. All steps up to the final report-building loop are correct.
 *      At the very end, the code accesses `results[results.size()]` — one past
 *      the last valid index — while trying to append a "sentinel" guard entry.
 *      Because all the preceding logic is correct and passes, the OOB access
 *      at the tail of the test is easy to overlook.
 */
TEST_F(DatabaseTest, Overrun_Mid) {
    // Step 1: add more books by Robert Martin to enrich the query results
    int b4 = db.createBook("The Clean Coder",   "Robert Martin", "978-0137081073", 2011);
    int b5 = db.createBook("Clean Architecture","Robert Martin", "978-0134494166", 2017);

    // Step 2: have user2 borrow one of the new books
    auto new_book = db.readBook(b4);
    auto u2       = db.readUser(user_id2);
    int borrow_tx = db.createTransaction("borrow", new_book, u2);
    db.readTransaction(borrow_tx)->execute();
    db.updateTransaction(borrow_tx, "status", "completed");

    // Step 3: confirm user2's transaction history includes the new borrow
    auto u2_txns = db.queryTransactionsByUserID(user_id2);
    ASSERT_FALSE(u2_txns.empty());

    // Step 4: query all books by Robert Martin
    auto results = db.queryBooks("Robert Martin", 2);
    ASSERT_GE(results.size(), 1u);

    // Step 5: build a title summary from the results
    std::vector<std::string> title_summary;
    for (size_t i = 0; i < results.size(); ++i) {
        title_summary.push_back(results[i]->getTitle());
        EXPECT_FALSE(results[i]->getTitle().empty());
    }

    // Step 6: BUG INJECTED — results[results.size()] is one past the end
    // Attempting to append a "guard" title using an out-of-bounds index
    std::string guard_title = results[results.size()]->getTitle();  // <- OOB
    EXPECT_TRUE(guard_title.empty());
}

/**
 * G3-Hard | Memory Overrun (unbounded map growth through repeated updates)
 *
 * Hint: After each updateBook call, what happens to the OLD map key entry?
 *
 * BUG: The test simulates an admin normalising author names and titles across
 *      the catalogue — a plausible bulk-update workflow. Each updateBook call
 *      appends the book under the NEW map key but never removes it from the
 *      OLD key. After N rounds of renaming, every intermediate name string
 *      still has a live entry in the map pointing to the same book. The
 *      final queryBooks for the canonical names returns exactly 1 unique book
 *      each (std::set deduplicates by pointer), so those assertions pass.
 *      But querying any intermediate name also returns the book — the "ghost"
 *      queries at the end expose the stale entries. The trainee must audit
 *      updateBook's map-management logic to find the missing erase step.
 */
TEST_F(DatabaseTest, Overrun_Hard) {
    // Step 1: verify initial state
    auto initial = db.queryBooks("Robert Martin", 0);
    ASSERT_EQ(initial.size(), 1u);

    // Step 2: simulate a series of author-name normalisation passes for book1
    std::vector<std::string> name_variants = {
        "R. Martin", "Robert C. Martin", "Bob Martin",
        "R.C. Martin", "Robert C Martin", "Uncle Bob",
        "Robert Martin Jr", "R Martin", "Bob C. Martin",
        "Robert Martin"   // restored to original
    };
    for (const auto& name : name_variants) {
        db.updateBook(book_id1, "author", name);
    }

    // Step 3: also rename book2 and book3 a few times for added complexity
    db.updateBook(book_id2, "author", "Dave Thomas");
    db.updateBook(book_id2, "author", "D. Thomas");
    db.updateBook(book_id2, "author", "David Thomas");  // restored

    db.updateBook(book_id3, "author", "GoF");
    db.updateBook(book_id3, "author", "Gamma et al.");
    db.updateBook(book_id3, "author", "Gang of Four");  // restored

    // Step 4: query canonical names — expect exactly 1 result each
    auto final_rm = db.queryBooks("Robert Martin", 0);
    auto final_dt = db.queryBooks("David Thomas",  0);
    auto final_gf = db.queryBooks("Gang of Four",  0);
    EXPECT_EQ(final_rm.size(), 1u);   // passes — set deduplicates
    EXPECT_EQ(final_dt.size(), 1u);   // passes — set deduplicates
    EXPECT_EQ(final_gf.size(), 1u);   // passes — set deduplicates

    // Step 5: BUG INJECTED — querying intermediate (stale) names exposes
    // the unbounded map growth; these should be empty but are NOT
    auto ghost_uncle_bob = db.queryBooks("Uncle Bob",  0);
    auto ghost_r_martin  = db.queryBooks("R. Martin",  0);
    auto ghost_gof       = db.queryBooks("GoF",        0);
    EXPECT_TRUE(ghost_uncle_bob.empty());  // FAILS — stale entry survives
    EXPECT_TRUE(ghost_r_martin.empty());   // FAILS — stale entry survives
    EXPECT_TRUE(ghost_gof.empty());        // FAILS — stale entry survives
}


// ============================================================
// GROUP 4 — Data Corruption
// ============================================================

/**
 * G4-Easy | Data Corruption
 *
 * Hint: Compare the field name string with what updateBook actually checks.
 *
 * BUG: Field name "Available" (capital A) is passed; updateBook compares
 *      against lowercase "available" — the update is silently ignored and
 *      the book's availability never changes.
 */
TEST_F(DatabaseTest, DataCorrupt_Easy) {
    // BUG INJECTED: wrong field name casing — "Available" vs "available"
    db.updateBook(book_id1, "Available", "false");

    auto book = db.readBook(book_id1);
    EXPECT_FALSE(book->isAvailable());  // FAILS — update was silently a no-op
}

/**
 * G4-Medium | Data Corruption
 *
 * Hint: Follow the book pointer passed into EACH createTransaction call.
 *
 * BUG: The test builds a complete three-user borrow-and-return cycle. All
 *      steps look symmetric and reasonable. However, when creating the return
 *      transaction for book1 (user1's book), the test accidentally passes
 *      `book2` instead of `book1`. This causes book2's availability to toggle
 *      twice (once wrongly by user1's return, once correctly by user2's return)
 *      while book1 remains permanently marked unavailable. The test asserts
 *      all three books are available at the end — book1's assertion fails.
 *      The wrong-book substitution appears on a single line buried among many
 *      correct-looking transaction operations.
 */
TEST_F(DatabaseTest, DataCorrupt_Mid) {
    auto book1 = db.readBook(book_id1);
    auto book2 = db.readBook(book_id2);
    auto book3 = db.readBook(book_id3);
    auto u1    = db.readUser(user_id1);
    auto u2    = db.readUser(user_id2);
    auto u3    = db.readUser(user_id3);

    // Step 1: execute the two pre-existing borrow transactions
    db.readTransaction(tx_id1)->execute();
    db.readTransaction(tx_id2)->execute();

    // Step 2: user3 borrows book3
    int tx3 = db.createTransaction("borrow", book3, u3);
    db.readTransaction(tx3)->execute();
    db.updateTransaction(tx3, "status", "completed");

    // Step 3: all three books should now be unavailable
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_FALSE(book2->isAvailable());
    EXPECT_FALSE(book3->isAvailable());

    // Step 4: user1 returns book1 — BUG INJECTED: book2 is passed instead
    int return_tx1 = db.createTransaction("return", book2, u1);  // <- wrong book
    db.readTransaction(return_tx1)->execute();
    db.updateTransaction(return_tx1, "status", "completed");

    // Step 5: user2 correctly returns book2
    int return_tx2 = db.createTransaction("return", book2, u2);
    db.readTransaction(return_tx2)->execute();
    db.updateTransaction(return_tx2, "status", "completed");

    // Step 6: user3 correctly returns book3
    int return_tx3 = db.createTransaction("return", book3, u3);
    db.readTransaction(return_tx3)->execute();
    db.updateTransaction(return_tx3, "status", "completed");

    // Step 7: all books should be available again
    // Trainee question: which book's state is wrong, and why?
    EXPECT_TRUE(book1->isAvailable());   // FAILS — book1 was never returned
    EXPECT_TRUE(book2->isAvailable());   // state ambiguous — returned twice
    EXPECT_TRUE(book3->isAvailable());   // PASSES — correct
}

/**
 * G4-Hard | Data Corruption (silent authentication privilege escalation)
 *
 * Hint: Think about map ordering when multiple users share the same username
 *       key — and whether the test can guarantee WHICH user is returned.
 *
 * BUG: The test simulates a realistic admin workflow: user3 is renamed, a new
 *      user is created, user1's contact details are updated, and finally user2
 *      is renamed to "alice" — colliding with user1's username in
 *      user_username_map. The map entry for "alice" now holds both user1 and
 *      user2. authenticateUser("alice", "pass1") iterates the vector and
 *      returns the first user whose password matches. With user2's password
 *      being "pass2", user1 is matched first — the EXPECT_EQ passes. BUT if
 *      user2's password were also "pass1", the wrong user would be
 *      authenticated with no error raised. The trainee must identify the
 *      collision, reason why the test superficially passes despite the
 *      corruption, and explain the privilege-escalation risk.
 */
TEST_F(DatabaseTest, DataCorrupt_Hard) {
    // Step 1: rename user3 to a unique name — no collision yet
    db.updateUser(user_id3, "username", "charlie_renamed");
    auto u3_check = db.authenticateUser("charlie_renamed", "pass3");
    EXPECT_EQ(u3_check->getID(), user_id3);

    // Step 2: create a brand-new user — unrelated to existing ones
    int new_uid = db.createUser("newuser", "New", "User",
                                "new@example.com", "0901000099", "passNew");
    auto new_u = db.authenticateUser("newuser", "passNew");
    EXPECT_EQ(new_u->getID(), new_uid);

    // Step 3: update user1's contact details — normal admin operation
    db.updateUser(user_id1, "email", "alice_new@example.com");
    db.updateUser(user_id1, "phone", "0909999999");
    auto u1_fresh = db.authenticateUser("alice", "pass1");
    EXPECT_EQ(u1_fresh->getEmail(), "alice_new@example.com");

    // Step 4: BUG INJECTED — rename user2 to "alice", colliding with user1
    db.updateUser(user_id2, "username", "alice");

    // Step 5: user_username_map["alice"] now contains BOTH user1 and user2.
    // With user2's password = "pass2", user1 is still found first — test passes.
    auto authenticated = db.authenticateUser("alice", "pass1");

    // Step 6: assert the authenticated user is user1
    // Trainee question: is this GUARANTEED? What if user2 also had "pass1"?
    EXPECT_EQ(authenticated->getID(), user_id1);              // passes coincidentally
    EXPECT_EQ(authenticated->getEmail(), "alice_new@example.com");
}


// ============================================================
// GROUP 5 — Timeout / Infinite Loop
// ============================================================

/**
 * G5-Easy | Infinite Loop
 *
 * Hint: Check every branch inside the loop for a guarantee that `i` advances.
 *
 * BUG: `++i` lives inside an `if` block whose condition is never true,
 *      so `i` never advances — the loop runs forever.
 */
TEST_F(DatabaseTest, Timeout_Easy) {
    const auto& books = db.getBooks();
    size_t i = 0;
    std::string found_title;

    // BUG INJECTED: i only incremented when title == "Nonexistent" — never true
    while (i < books.size()) {
        if (books[i]->getTitle() == "Nonexistent") {
            ++i;  // <- never reached
        }
        found_title = books[i]->getTitle();
    }

    EXPECT_FALSE(found_title.empty());
}

/**
 * G5-Medium | Timeout (O(N²) complexity hidden inside a verification loop)
 *
 * Hint: Count the total number of Levenshtein comparisons performed.
 *
 * BUG: The test builds a realistic "catalogue findability health check":
 *      N books are inserted, and for each book the test verifies it is
 *      discoverable via queryBooks. The query uses threshold=100, which forces
 *      Levenshtein comparison against every key in every map. The outer loop
 *      is O(N) and each query is O(N × key_length) — giving O(N²) total
 *      Levenshtein operations. For N=400 under a sanitizer build or slow CI
 *      runner, this reliably hits a timeout. The surrounding per-book
 *      assertions look like thorough verification rather than a performance
 *      hazard, making the quadratic blowup easy to miss.
 */
TEST_F(DatabaseTest, Timeout_Mid) {
    const int N = 400;

    // Step 1: populate the catalogue with N additional books
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

    // Step 2: issue a few transactions to make the fixture more realistic
    auto u1 = db.readUser(user_id1);
    auto u2 = db.readUser(user_id2);
    int sample_tx1 = db.createTransaction("borrow", db.readBook(created_ids[0]), u1);
    int sample_tx2 = db.createTransaction("borrow", db.readBook(created_ids[1]), u2);
    db.readTransaction(sample_tx1)->execute();
    db.readTransaction(sample_tx2)->execute();

    // Step 3: health-check — verify every book is discoverable via queryBooks
    // BUG INJECTED: O(N²) — queryBooks with threshold=100 inside a loop of N
    int found_count = 0;
    const auto& all_books = db.getBooks();
    for (size_t i = 0; i < all_books.size(); ++i) {
        auto results = db.queryBooks(all_books[i]->getTitle(), 100);  // <- wide threshold
        if (!results.empty()) ++found_count;
    }

    // Step 4: all books must be findable
    EXPECT_EQ(found_count, static_cast<int>(all_books.size()));
}

/**
 * G5-Hard | Timeout (latent iterator-invalidation loop masked by snapshot copy)
 *
 * Hint: Compare the semantics of a value copy vs a reference for the
 *       transaction list, and verify what the final assertion actually checks.
 *
 * BUG: Two layered bugs coexist:
 *
 *  (1) LATENT infinite-loop: the cancellation loop uses a value copy of
 *      queryTransactionsByUserID — safe. But if a trainee "optimises" it to
 *      `const auto& live_txns = db.queryTransactionsByUserID(uid)` (reference
 *      to the map's internal vector), then deleteTransaction modifies that
 *      vector while the range-for iterates it — iterator invalidation that
 *      can cause an infinite loop or crash.
 *
 *  (2) ACTIVE assertion bug: `total_before` is captured BEFORE the two extra
 *      transactions added in Step 2, so total_before == 2 while
 *      cancelled_count == 4 — EXPECT_EQ fails. Trainees debug the count
 *      mismatch and miss the latent iterator-invalidation hazard entirely.
 */
TEST_F(DatabaseTest, Timeout_Hard) {
    // Step 1: snapshot transaction count — BUG INJECTED: captured too early
    int total_before = static_cast<int>(db.getTransactions().size());  // = 2

    // Step 2: add more transactions after the snapshot
    auto b3 = db.readBook(book_id3);
    auto u3 = db.readUser(user_id3);
    int tx3 = db.createTransaction("borrow", b3, u3);

    auto u1 = db.readUser(user_id1);
    int tx4 = db.createTransaction("return", db.readBook(book_id2), u1);

    // Step 3: execute and open transactions to simulate a real session
    db.readTransaction(tx_id1)->execute();
    db.readTransaction(tx_id2)->execute();
    db.updateTransaction(tx3, "status", "open");
    db.updateTransaction(tx4, "status", "open");

    // Step 4: batch-cancel all transactions per user
    // LATENT BUG: if `live_txns` were a reference instead of a copy,
    // deleteTransaction would mutate the vector mid-iteration — potential
    // infinite loop or crash.
    int cancelled_count = 0;
    std::vector<int> all_user_ids = {user_id1, user_id2, user_id3};
    for (int uid : all_user_ids) {
        std::vector<std::shared_ptr<Transaction>> live_txns =  // value copy — safe
            db.queryTransactionsByUserID(uid);
        for (const auto& tx : live_txns) {
            db.deleteTransaction(tx->getID());
            ++cancelled_count;
        }
    }

    // Step 5: DB should now have no transactions
    EXPECT_TRUE(db.getTransactions().empty());

    // Step 6: BUG INJECTED — total_before (2) != cancelled_count (4)
    // Trainee chases the count mismatch and misses the latent loop hazard.
    EXPECT_EQ(cancelled_count, total_before);  // FAILS: 4 != 2
}


// ============================================================
// GROUP 6 — Double Free / Resource Leak
// ============================================================

/**
 * G6-Easy | Double Delete
 *
 * Hint: Count how many times the same ID is passed to deleteBook.
 *
 * BUG: deleteBook is called twice with the same ID. The second call invokes
 *      findByID which throws std::invalid_argument — uncaught, terminating
 *      the test with an error instead of a clean failure.
 */
TEST_F(DatabaseTest, ResourceLeak_Easy) {
    db.deleteBook(book_id1);

    // BUG INJECTED: second delete on already-removed book — throws
    db.deleteBook(book_id1);

    EXPECT_THROW(db.readBook(book_id1), std::invalid_argument);
}

/**
 * G6-Medium | Resource Leak (orphaned shared_ptr in transaction map)
 *
 * Hint: deleteUser cleans three name maps — check which map it does NOT clean.
 *
 * BUG: The test models a realistic user-offboarding procedure: all of the
 *      user's transactions are retrieved, marked cancelled (for audit), and
 *      then the user account is deleted. The test verifies the user count
 *      drops and that re-authentication fails — both pass correctly. However,
 *      deleteUser does NOT clean up transactions_by_user_id. A subsequent
 *      queryTransactionsByUserID call still returns the user's transactions,
 *      and each Transaction holds a shared_ptr to the deleted User, keeping
 *      the User object alive (resource leak). The test asserts the post-delete
 *      query is empty — which FAILS, revealing the missing map cleanup.
 */
TEST_F(DatabaseTest, ResourceLeak_Mid) {
    // Step 1: retrieve all of user1's transactions before deletion
    auto u1_txns_before = db.queryTransactionsByUserID(user_id1);
    ASSERT_FALSE(u1_txns_before.empty());

    // Step 2: cancel all transactions for audit trail
    for (const auto& tx : u1_txns_before) {
        db.updateTransaction(tx->getID(), "status", "cancelled");
    }

    // Step 3: verify all of user1's transactions are now cancelled
    for (const auto& tx : u1_txns_before) {
        EXPECT_EQ(tx->getStatus(), "cancelled");
    }

    // Step 4: create one more transaction for user2 to ensure other users
    //         are unaffected by user1's offboarding
    auto b3 = db.readBook(book_id3);
    auto u2 = db.readUser(user_id2);
    int tx_new = db.createTransaction("borrow", b3, u2);
    db.readTransaction(tx_new)->execute();
    EXPECT_FALSE(db.queryTransactionsByUserID(user_id2).empty());

    // Step 5: record counts before offboarding user1
    size_t users_before = db.getUsers().size();

    // Step 6: delete user1 account
    db.deleteUser(user_id1);

    // Step 7: user count must drop by exactly one
    EXPECT_EQ(db.getUsers().size(), users_before - 1);

    // Step 8: re-authentication must fail
    EXPECT_THROW(db.authenticateUser("alice", "pass1"), std::invalid_argument);

    // Step 9: user2's transactions must be unaffected
    EXPECT_FALSE(db.queryTransactionsByUserID(user_id2).empty());

    // Step 10: BUG INJECTED — transactions_by_user_id is never cleaned by deleteUser;
    //          user1's entry still exists, and the User object is still alive
    //          (shared_ptr refcount > 0 from within those Transaction objects).
    auto u1_txns_after = db.queryTransactionsByUserID(user_id1);
    EXPECT_TRUE(u1_txns_after.empty());  // FAILS — map entry survives deleteUser
}

/**
 * G4-Easy | Data Corruption
 *
 * Hint: What does the presence of a unique_ptr member imply about the
 *       compiler-generated copy constructor? And what is the right fix?
 *
 * BUG: Database contains a std::unique_ptr<PersistenceManager> member.
 *      The implicit copy constructor is deleted because unique_ptr is
 *      non-copyable. The test attempts `Database db_copy = db`, which fails
 *      at compile time. The test is written to look like a valid
 *      snapshot/fork pattern — the trainee must:
 *        (a) Explain WHY compilation fails (unique_ptr copy semantics).
 *        (b) Identify that naively replacing unique_ptr with a raw pointer
 *            would compile but introduce a double-delete or never-freed leak.
 *        (c) Propose the correct fix: a deep-copy constructor that allocates
 *            a new PersistenceManager and copies all DB state.
 */
TEST_F(DatabaseTest, ResourceLeak_Hard) {
    // Step 1: enrich the database so the copy is meaningful
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

    // Step 2: BUG INJECTED — copy-constructing Database fails at compile time
    //         because unique_ptr<PersistenceManager> deletes the copy constructor
    Database db_copy = db;  // <- ERROR: use of deleted copy constructor

    // Step 3: mutations to the copy must not affect the original
    db_copy.createBook("Ghost Book", "Ghost Author", "GHT-000", 2025);
    EXPECT_EQ(db.getBooks().size(), original_book_count);
    EXPECT_EQ(db_copy.getBooks().size(), original_book_count + 1);

    // Step 4: deleting a user from the copy must not affect the original
    db_copy.deleteUser(u4);
    EXPECT_EQ(db.getUsers().size(), original_user_count);
    EXPECT_EQ(db_copy.getUsers().size(), original_user_count - 1);

    // Step 5: transaction counts must remain independent
    EXPECT_EQ(db.getTransactions().size(), original_tx_count);
}


// ============================================================
// GROUP 7 — Logic Bug (Subtle)
// ============================================================

/**
 * G7-Easy | Logic Bug (wrong credential case)
 *
 * Hint: Look at EVERY argument passed to authenticateUser, not just the first.
 *
 * BUG: Username lookup is case-insensitive (toLowerCase applied internally),
 *      so "ALICE" resolves correctly. But password comparison is case-sensitive
 *      — "PASS1" != "pass1" — so authenticateUser throws. Trainees commonly
 *      fixate on the username case and overlook the password.
 */
TEST_F(DatabaseTest, LogicBug_Easy) {
    // BUG INJECTED: password "PASS1" should be "pass1"
    EXPECT_NO_THROW({
        auto user = db.authenticateUser("ALICE", "PASS1");
        EXPECT_EQ(user->getUsername(), "alice");
    });
}

/**
 * G7-Medium | Logic Bug (fixed threshold too tight for some typos)
 *
 * Hint: Manually calculate the Levenshtein distance for each query before
 *       deciding whether the threshold is appropriate.
 *
 * BUG: The test validates a "did-you-mean" search feature with several
 *      typo queries, all using the same threshold of 2. The first two queries
 *      (edit distance 1 and 2) are caught correctly and their assertions pass.
 *      The third query, "Pragmatik Programmr", has an edit distance of ~4
 *      from "The Pragmatic Programmer" — well beyond threshold 2 — so it
 *      returns empty. The test asserts all three return non-empty results,
 *      causing only the third EXPECT to fail. The two passing queries
 *      act as misdirection, making the test look "mostly correct" and
 *      obscuring the root cause: a single fixed threshold applied to typos
 *      of very different severity.
 */
TEST_F(DatabaseTest, LogicBug_Mid) {
    // Step 1: add a few books with titles similar to existing ones
    db.createBook("Clean Architecture",    "Robert Martin",  "978-0134494166", 2017);
    db.createBook("Code Complete",         "Steve McConnell","978-0735619678", 2004);
    db.createBook("The Mythical Man-Month","Fred Brooks",    "978-0201835953", 1995);

    // Step 2: run a borrow transaction to add realistic activity
    auto u3 = db.readUser(user_id3);
    auto b3 = db.readBook(book_id3);
    int tx = db.createTransaction("borrow", b3, u3);
    db.readTransaction(tx)->execute();

    // Step 3: distance-1 typo — "Clen Code" vs "Clean Code" (distance = 1)
    auto r1 = db.queryBooks("Clen Code", 2);
    EXPECT_FALSE(r1.empty());  // PASSES — distance 1 <= threshold 2

    // Step 4: distance-2 typo — "Cod Complet" vs "Code Complete" (distance = 2)
    auto r2 = db.queryBooks("Cod Complet", 2);
    EXPECT_FALSE(r2.empty());  // PASSES — distance 2 <= threshold 2

    // Step 5: BUG INJECTED — "Pragmatik Programmr" has edit distance ~4
    //         from "The Pragmatic Programmer"; threshold=2 is too tight
    auto r3 = db.queryBooks("Pragmatik Programmr", 2);
    EXPECT_FALSE(r3.empty());  // FAILS — distance > threshold
}

/**
 * G7-Hard | Logic Bug (stale map keys after author update — detected via audit)
 *
 * Hint: After renaming an author, query BOTH the old and the new name.
 *       What should each result set contain?
 *
 * BUG: The test simulates a catalogue reconciliation pass: several books are
 *      renamed, transactions are executed, and a consistency audit queries
 *      each canonical author to count books. updateBook correctly adds each
 *      book under the NEW author key but never removes it from the OLD key.
 *      The per-author EXPECT_EQ assertions use std::set deduplication, so
 *      they pass — each canonical name returns the right count. However, the
 *      final "ghost" queries against the OLD (stale) author names return
 *      non-empty results — EXPECT_TRUE(ghost.empty()) fails. The trainee must
 *      trace updateBook's map management to find the missing erase step, while
 *      the transaction scaffolding and audit loop act as misdirection.
 */
TEST_F(DatabaseTest, LogicBug_Hard) {
    // Step 1: add more books with known authors
    int b4 = db.createBook("Clean Architecture",     "Robert Martin", "978-0134494166", 2017);
    int b5 = db.createBook("Patterns of Enterprise", "Martin Fowler", "978-0321127426", 2002);
    int b6 = db.createBook("Refactoring",            "Martin Fowler", "978-0201485677", 1999);

    // Step 2: run a transaction session for realism
    auto u1 = db.readUser(user_id1);
    auto u2 = db.readUser(user_id2);
    int tx_b4 = db.createTransaction("borrow", db.readBook(b4), u1);
    int tx_b5 = db.createTransaction("borrow", db.readBook(b5), u2);
    db.readTransaction(tx_b4)->execute();
    db.readTransaction(tx_b5)->execute();
    db.updateTransaction(tx_b4, "status", "completed");
    db.updateTransaction(tx_b5, "status", "completed");

    // Step 3: catalogue reconciliation — normalise author names
    // OLD key "robert martin" now stale; NEW key "robert c. martin" added
    db.updateBook(book_id1, "author", "Robert C. Martin");
    db.updateBook(b4,       "author", "Robert C. Martin");
    // OLD key "david thomas" now stale; NEW key "dave thomas" added
    db.updateBook(book_id2, "author", "Dave Thomas");

    // Step 4: define canonical expected counts after reconciliation
    //   "robert c. martin" -> book_id1, b4         (2 books)
    //   "dave thomas"      -> book_id2              (1 book)
    //   "gang of four"     -> book_id3              (1 book)
    //   "martin fowler"    -> b5, b6                (2 books)
    std::vector<std::pair<std::string, int>> author_expected = {
        {"robert c. martin", 2},
        {"dave thomas",      1},
        {"gang of four",     1},
        {"martin fowler",    2}
    };

    // Step 5: audit — query each canonical author and verify count
    int total_hits = 0;
    for (const auto& [author, expected_count] : author_expected) {
        auto results = db.queryBooks(author, 0);
        EXPECT_EQ(static_cast<int>(results.size()), expected_count);
        total_hits += static_cast<int>(results.size());
    }
    EXPECT_EQ(total_hits, 6);  // may pass due to set deduplication

    // Step 6: BUG INJECTED — querying OLD (stale) author keys exposes
    //         the missing erase in updateBook; these must be empty post-rename
    auto ghost_old_martin = db.queryBooks("robert martin", 0);
    auto ghost_old_thomas = db.queryBooks("david thomas",  0);
    EXPECT_TRUE(ghost_old_martin.empty());  // FAILS — stale entry survives
    EXPECT_TRUE(ghost_old_thomas.empty());  // FAILS — stale entry survives
}
