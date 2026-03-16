/**
 * UserTest.cpp
 *
 * GTest suite for User.cpp / User.hpp — designed as a TRAINEE EVALUATION exercise.
 *
 * Each test group targets a specific class of bug intentionally planted in the
 * test logic. Trainees must:
 *   1. Dry-run / read the test code carefully.
 *   2. Identify the root cause of the injected defect.
 *   3. Propose a correct fix and explain the underlying C++ mechanism.
 *
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │ GROUP │ BUG TYPE                          │ TEST NAMES                   │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │  G1   │ Segmentation Fault                │ SegFault_Easy/Mid/Hard       │
 * │  G2   │ Use After Free                    │ UseAfterFree_Easy/Mid/Hard   │
 * │  G3   │ Data Corruption (setter / field)  │ DataCorrupt_Easy/Mid/Hard    │
 * │  G4   │ Logic Bug (borrow / return state) │ LogicBug_Easy/Mid/Hard       │
 * │  G5   │ Regex / Validation Bug            │ RegexBug_Easy/Mid/Hard       │
 * │  G6   │ Index / Memory Overrun            │ Overrun_Easy/Mid/Hard        │
 * │  G7   │ Double Operation / Inconsistency  │ DoubleOp_Easy/Mid/Hard       │
 * └──────────────────────────────────────────────────────────────────────────┘
 *
 * Difficulty scale:
 *   Easy   — bug is immediately visible on first read.
 *   Medium — requires tracing data flow across 6–10 lines; verbose setup
 *            is intentional misdirection.
 *   Hard   — test appears complete and correct; defect is contextual,
 *            rooted in a subtle C++ or domain-logic semantic.
 *
 * NOTE ON VALID PHONE FORMAT:
 *   User's constructor regex: ^[+]{1}(?:[0-9\-\(\)\/\.\s?]){6,15}[0-9]{1}$
 *   All valid test phones use international format, e.g. "+84901000001".
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include "User.hpp"
#include "Book.hpp"

// ============================================================
// Helper factory — avoids repeating valid constructor arguments
// ============================================================
static std::shared_ptr<User> makeUser(
    int id,
    const std::string& username  = "alice",
    const std::string& forename  = "Alice",
    const std::string& surname   = "Smith",
    const std::string& email     = "alice@example.com",
    const std::string& phone     = "+84901000001",
    const std::string& password  = "pass1")
{
    return std::make_shared<User>(id, username, forename, surname, email, phone, password);
}

static std::shared_ptr<Book> makeBook(
    int id,
    const std::string& title  = "Clean Code",
    const std::string& author = "Robert Martin",
    const std::string& isbn   = "978-0132350884",
    int year = 2008,
    bool available = true)
{
    return std::make_shared<Book>(id, title, author, isbn, year, available);
}

// ============================================================
// Shared fixture
// ============================================================
class UserTest : public ::testing::Test {
protected:
    std::shared_ptr<User> user1;
    std::shared_ptr<User> user2;
    std::shared_ptr<User> user3;

    std::shared_ptr<Book> book1;
    std::shared_ptr<Book> book2;
    std::shared_ptr<Book> book3;

    void SetUp() override {
        user1 = makeUser(1, "alice",   "Alice",   "Smith",  "alice@example.com",   "+84901000001", "pass1");
        user2 = makeUser(2, "bob",     "Bob",     "Jones",  "bob@example.com",     "+84901000002", "pass2");
        user3 = makeUser(3, "charlie", "Charlie", "Brown",  "charlie@example.com", "+84901000003", "pass3");

        book1 = makeBook(1, "Clean Code",                "Robert Martin", "978-0132350884", 2008, true);
        book2 = makeBook(2, "The Pragmatic Programmer",  "David Thomas",  "978-0201616224", 1999, true);
        book3 = makeBook(3, "Design Patterns",           "Gang of Four",  "978-0201633610", 1994, true);
    }
};


// ============================================================
// GROUP 1 — Segmentation Fault (null / dangling pointer)
// ============================================================

/**
 * G1-Easy | Segmentation Fault
 *
 * Hint: Inspect what value `null_book` holds when it is passed to borrowBook.
 *
 * BUG: A nullptr shared_ptr<Book> is passed directly to user1->borrowBook().
 *      Inside borrowBook(), the first line calls `book->borrowBook()` —
 *      dereferencing a null pointer and causing an immediate segfault.
 */
TEST_F(UserTest, SegFault_Easy) {
    std::shared_ptr<Book> null_book = nullptr;

    // BUG INJECTED: null_book passed to borrowBook — crashes on book->borrowBook()
    EXPECT_TRUE(user1->borrowBook(null_book));  // <- segfault inside borrowBook()
}

/**
 * G1-Medium | Segmentation Fault
 *
 * Hint: Trace the exact lifetime of `raw_ptr` across all steps below.
 *
 * BUG: The test builds a realistic borrow workflow for three users: each user
 *      borrows a book, statuses are verified, and a "report" iterates all
 *      users' borrowed books. During the report loop, a raw pointer `raw_ptr`
 *      is captured from user1's borrowed-book list. Immediately after capture,
 *      user1 returns the book — Book::returnBook() is called and the
 *      shared_ptr is erased from borrowed_books. The Book object itself may
 *      still be alive (other shared_ptrs exist), but the local variable
 *      holding the pointer is then reset(). The very next line reads through
 *      `raw_ptr`, which is now dangling if the only remaining reference was
 *      the one inside borrowed_books that was just erased. The surrounding
 *      multi-user setup is misdirection.
 */
TEST_F(UserTest, SegFault_Mid) {
    // Step 1: all three users borrow one book each
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user3->borrowBook(book3));

    // Step 2: verify each user holds exactly one borrowed book
    EXPECT_EQ(user1->getBorrowedBooks().size(), 1u);
    EXPECT_EQ(user2->getBorrowedBooks().size(), 1u);
    EXPECT_EQ(user3->getBorrowedBooks().size(), 1u);

    // Step 3: simulate a report — iterate user1's borrowed books
    auto borrowed = user1->getBorrowedBooks();  // returns a COPY
    ASSERT_FALSE(borrowed.empty());

    // Step 4: capture a raw pointer from the copy before it is erased
    const Book* raw_ptr = borrowed.front().get();

    // Step 5: user1 returns the book — erases the shared_ptr from borrowed_books
    ASSERT_TRUE(user1->returnBook(book1));

    // Step 6: reset the local copy — only surviving reference may now be gone
    // BUG INJECTED: `borrowed` is reset; if book1 had refcount 1 inside
    // `borrowed`, the object is destroyed and raw_ptr dangles.
    borrowed.clear();  // drops the shared_ptr inside the copy

    // Step 7: dereference raw_ptr — potential segfault if Book was freed
    // Trainee question: who else holds a reference to book1 at this point?
    EXPECT_EQ(raw_ptr->getTitle(), "Clean Code");  // <- potential dangling dereference
}

/**
 * G1-Hard | Segmentation Fault (stale raw ptr after vector reallocation)
 *
 * Hint: Think about what getBorrowedBooks() actually returns and where
 *       the raw pointer is pointing AFTER additional borrows.
 *
 * BUG: The test captures a raw Book* from the first element of the vector
 *      returned by getBorrowedBooks(). Since getBorrowedBooks() returns a
 *      VALUE COPY of the internal vector, `first_copy` is a temporary vector
 *      that lives only as long as the variable. The test correctly keeps
 *      `first_copy` alive. However, after borrowing 30 more books, the test
 *      calls getBorrowedBooks() AGAIN and captures a pointer into the NEW
 *      copy's front element — discarding the old copy. The raw pointer
 *      `stable_ptr` now points into a destroyed temporary vector's managed
 *      object. Because shared_ptr's managed object is heap-allocated
 *      separately, the Book itself is still alive — BUT the test then
 *      intentionally resets `second_copy` before reading `stable_ptr`,
 *      making the final read a genuine dangling dereference if the book's
 *      only remaining strong reference was inside `second_copy`.
 */
TEST_F(UserTest, SegFault_Hard) {
    // Step 1: borrow book1 as the initial "anchor" book
    ASSERT_TRUE(user1->borrowBook(book1));

    // Step 2: capture a raw pointer from the first getBorrowedBooks() copy
    auto first_copy = user1->getBorrowedBooks();
    ASSERT_EQ(first_copy.size(), 1u);
    const Book* stable_ptr = first_copy.front().get();
    std::string expected_title = first_copy.front()->getTitle();

    // Step 3: borrow 30 more books to stress the internal vector
    std::vector<std::shared_ptr<Book>> extra_books;
    for (int i = 10; i < 40; ++i) {
        auto b = makeBook(i, "Extra Book " + std::to_string(i),
                          "Author " + std::to_string(i),
                          "ISBN-"   + std::to_string(i), 2000 + i, true);
        extra_books.push_back(b);
        ASSERT_TRUE(user1->borrowBook(b));
    }

    // Step 4: take a second snapshot — discard first_copy
    auto second_copy = user1->getBorrowedBooks();
    ASSERT_GE(second_copy.size(), 31u);
    first_copy.clear();  // first_copy's shared_ptrs dropped

    // Step 5: BUG INJECTED — reset second_copy which held the last reference
    // to the Book objects within that snapshot
    second_copy.clear();  // <- drops shared_ptrs inside second_copy

    // Step 6: stable_ptr may now be dangling if no other owner holds book1
    // Trainee question: is book1 still alive? Who holds the last reference?
    EXPECT_EQ(stable_ptr->getTitle(), expected_title);  // <- potential dangling dereference
}


// ============================================================
// GROUP 2 — Use After Free
// ============================================================

/**
 * G2-Easy | Use After Free
 *
 * Hint: Look at what returnBook does to the shared_ptr inside borrowed_books,
 *       and what happens to `book_ref` afterwards.
 *
 * BUG: `book_ref` is a reference-alias of the first element inside the vector
 *      returned by getBorrowedBooks(). Since getBorrowedBooks() returns a copy,
 *      `book_ref` actually aliases an element of a temporary — after the
 *      temporary goes out of scope (immediately, since it's not bound to a
 *      named variable), `book_ref` is dangling. Reading through it is UB.
 */
TEST_F(UserTest, UseAfterFree_Easy) {
    user1->borrowBook(book1);

    // BUG INJECTED: binding a reference to an element of a temporary vector
    // The temporary returned by getBorrowedBooks() is destroyed immediately,
    // making book_ref a dangling reference.
    const std::shared_ptr<Book>& book_ref = user1->getBorrowedBooks().front();

    // book_ref is dangling here
    EXPECT_EQ(book_ref->getTitle(), "Clean Code");  // <- UB / dangling ref
}

/**
 * G2-Medium | Use After Free
 *
 * Hint: Follow the lifecycle of the shared_ptr inside `snapshot` and what
 *       happens to the Book object after returnBook is called on it.
 *
 * BUG: The test models a realistic checkout-and-return cycle for three users.
 *      user1 borrows book1, the test snapshots user1's borrowed list for
 *      later verification, then user1 returns book1. At this point the
 *      shared_ptr inside user1's borrowed_books is erased — but `snapshot`
 *      still holds its own shared_ptr to book1, keeping it alive. The test
 *      then calls `snapshot[0].reset()` to "clean up" — this drops the last
 *      remaining external reference. Immediately after, the code reads
 *      `raw_after_reset` which was captured from `snapshot[0].get()` BEFORE
 *      the reset, making it a dangling pointer to a now-destroyed Book.
 *      The extensive multi-user setup before the critical section is
 *      misdirection.
 */
TEST_F(UserTest, UseAfterFree_Mid) {
    // Step 1: all users borrow their respective books
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user3->borrowBook(book3));

    // Step 2: verify checkout status for all users
    EXPECT_TRUE(user1->checkOutStatus(book1));
    EXPECT_TRUE(user2->checkOutStatus(book2));
    EXPECT_TRUE(user3->checkOutStatus(book3));

    // Step 3: snapshot user1's borrowed list for audit purposes
    auto snapshot = user1->getBorrowedBooks();
    ASSERT_EQ(snapshot.size(), 1u);

    // Step 4: capture raw pointer from the snapshot for later comparison
    const Book* raw_after_reset = snapshot[0].get();
    EXPECT_EQ(raw_after_reset->getTitle(), "Clean Code");

    // Step 5: user1 returns book1 — erases shared_ptr from borrowed_books
    ASSERT_TRUE(user1->returnBook(book1));
    EXPECT_FALSE(user1->checkOutStatus(book1));

    // Step 6: user2 and user3 continue their sessions normally
    ASSERT_TRUE(user2->borrowBook(book3));  // user2 borrows a second book
    EXPECT_EQ(user2->getBorrowedBooks().size(), 2u);

    // Step 7: "clean up" the audit snapshot
    // BUG INJECTED: this drops the last external shared_ptr to book1
    // (book1 itself in SetUp still exists — but the test resets it too below)
    snapshot[0].reset();  // <- drops snapshot's reference to book1
    book1.reset();        // <- drops SetUp's shared_ptr — book1 may be destroyed

    // Step 8: raw_after_reset is now potentially dangling
    // Trainee question: who holds book1 at this point? Is raw_after_reset safe?
    EXPECT_EQ(raw_after_reset->getTitle(), "Clean Code");  // <- dangling dereference
}

/**
 * G2-Hard | Use After Free (iterator invalidation through erase during traversal)
 *
 * Hint: Trace exactly what returnBook does to the borrowed_books vector
 *       while the outer loop is still iterating over a snapshot of it.
 *
 * BUG: The test simulates a bulk-return operation: user1 borrows 5 books,
 *      then a "batch return" loop is run over the getBorrowedBooks() snapshot.
 *      Inside the loop, `user1->returnBook(b)` is called correctly — so far
 *      safe. However, at the end the test attempts to verify the result by
 *      iterating the SAME `snapshot` vector and calling checkOutStatus, which
 *      is fine. The real bug is subtler: during the batch-return loop, the
 *      test also stores the result of `user1->getBorrowedBooks()` into a
 *      reference variable `live_view` on each iteration — except it is NOT a
 *      reference; it is a new copy each time, making the loop O(N²) in copies.
 *      But the actual defect is in the POST-loop verification: the test
 *      asserts `live_view.empty()` using the LAST captured copy, which was
 *      taken when one book was still remaining — so live_view.size() == 1,
 *      not 0. The assertion fails for the wrong (non-obvious) reason.
 */
TEST_F(UserTest, UseAfterFree_Hard) {
    // Step 1: user1 borrows 5 books in sequence
    std::vector<std::shared_ptr<Book>> all_books = {book1, book2, book3};
    auto b4 = makeBook(4, "Refactoring",           "Martin Fowler", "978-0201485677", 1999, true);
    auto b5 = makeBook(5, "Clean Architecture",    "Robert Martin", "978-0134494166", 2017, true);
    all_books.push_back(b4);
    all_books.push_back(b5);

    for (auto& b : all_books) {
        ASSERT_TRUE(user1->borrowBook(b));
    }
    ASSERT_EQ(user1->getBorrowedBooks().size(), 5u);

    // Step 2: snapshot the list for batch return
    auto snapshot = user1->getBorrowedBooks();

    // Step 3: batch-return loop — captures a new "live_view" on each iteration
    // BUG INJECTED: `live_view` is captured INSIDE the loop, so on the last
    // iteration (when 1 book remains), it captures a 1-element vector.
    // After the loop ends, live_view holds the state from the SECOND-TO-LAST
    // iteration, not the final empty state.
    std::vector<std::shared_ptr<Book>> live_view;
    for (size_t i = 0; i < snapshot.size(); ++i) {
        live_view = user1->getBorrowedBooks();  // captures current state mid-loop
        bool returned = user1->returnBook(snapshot[i]);
        EXPECT_TRUE(returned);
    }

    // Step 4: verify all books are returned via checkOutStatus
    for (const auto& b : all_books) {
        EXPECT_FALSE(user1->checkOutStatus(b));
    }

    // Step 5: verify borrowed list is empty
    // Trainee question: what does live_view contain at this point?
    // BUG INJECTED: live_view was last assigned when snapshot[3] was still
    // borrowed (before the final returnBook call), so live_view.size() == 1.
    EXPECT_TRUE(live_view.empty());            // FAILS — live_view has 1 element
    EXPECT_EQ(user1->getBorrowedBooks().size(), 0u);  // passes — actual state correct
}


// ============================================================
// GROUP 3 — Data Corruption (setter / field bug)
// ============================================================

/**
 * G3-Easy | Data Corruption
 *
 * Hint: Read setPhoneNumber in User.cpp and check which variable is validated.
 *
 * BUG: User::setPhoneNumber validates `phone_number` (the OLD stored value)
 *      instead of `new_phone_number` (the argument). If the old value passes
 *      the regex (which it does for valid users), ANY new string — including
 *      a completely invalid one — is accepted without error, silently
 *      corrupting the phone field. The test expects a throw, but none comes.
 */
TEST_F(UserTest, DataCorrupt_Easy) {
    // Current phone "+84901000001" is valid — setPhoneNumber validates OLD value,
    // so the invalid new value passes through without throwing.
    // BUG INJECTED: expects throw — but setPhoneNumber validates wrong variable
    EXPECT_THROW(
        user1->setPhoneNumber("not-a-phone-number"),
        std::invalid_argument
    );  // FAILS — no exception thrown; corrupt value is silently stored
}

/**
 * G3-Medium | Data Corruption
 *
 * Hint: Trace every setPhoneNumber call and figure out WHICH value is being
 *       validated in each call, given the real implementation.
 *
 * BUG: The test simulates an admin correcting user contact details across
 *      three users. Multiple setPhoneNumber calls are made in a realistic
 *      sequence. Because setPhoneNumber always validates the CURRENTLY STORED
 *      phone (not the new argument), the following happens:
 *        - Call 1 on user1: old phone valid → any new value accepted → user1
 *          gets an invalid phone "+00" silently stored.
 *        - Call 2 on user1: old phone is now "+00" (invalid) → regex fails →
 *          valid new phone "+84999999991" is REJECTED with an exception.
 *        - Call on user2: old phone valid → invalid phone stored silently.
 *      The test wraps each call in EXPECT_NO_THROW, which only highlights the
 *      second call's unexpected throw. But the root cause — all first-call
 *      silent corruptions — is the deeper bug the trainee must identify.
 */
TEST_F(UserTest, DataCorrupt_Mid) {
    // Step 1: verify initial phone numbers are correctly stored
    EXPECT_EQ(user1->getPhoneNumber(), "+84901000001");
    EXPECT_EQ(user2->getPhoneNumber(), "+84901000002");
    EXPECT_EQ(user3->getPhoneNumber(), "+84901000003");

    // Step 2: admin sets a "temporary placeholder" phone for user1 while
    //         verifying the new number via a separate channel
    // BUG INJECTED (silent): old phone valid → "+00" stored without error
    EXPECT_NO_THROW(user1->setPhoneNumber("+00"));  // passes — should throw

    // Step 3: admin confirms the real new number and applies it
    // BUG INJECTED (throws unexpectedly): old phone is now "+00" (invalid)
    // → regex rejects it → valid new number is blocked
    EXPECT_NO_THROW(                                // FAILS — throws here
        user1->setPhoneNumber("+84999999991")
    );

    // Step 4: admin also updates user2's phone — old phone valid → silent corrupt
    EXPECT_NO_THROW(user2->setPhoneNumber("INVALID-PHONE"));
    EXPECT_EQ(user2->getPhoneNumber(), "INVALID-PHONE");  // silently accepted

    // Step 5: update user3 normally — should succeed
    EXPECT_NO_THROW(user3->setPhoneNumber("+84911111111"));
    EXPECT_EQ(user3->getPhoneNumber(), "+84911111111");

    // Step 6: verify final states
    // Trainee question: what is user1->getPhoneNumber() at this point, and why?
    EXPECT_EQ(user1->getPhoneNumber(), "+84999999991");  // FAILS — still "+00"
    EXPECT_EQ(user2->getPhoneNumber(), "+84901000002");  // FAILS — silently corrupted
}

/**
 * G3-Hard | Data Corruption (setPhoneNumber validates old, combined with
 *           operator== only comparing user_id)
 *
 * Hint: Consider what User::operator== compares, then think about what
 *       happens when you use User objects in a std::set or std::vector lookup
 *       after field-level corruption.
 *
 * BUG: The test models a user-registry update flow where a manager fetches a
 *      user, updates several fields, then "verifies" the update by comparing
 *      the updated user against the original using operator==. Since
 *      operator== only compares user_id, two User objects with the same ID
 *      but completely different field values are "equal" — the verify step
 *      always passes regardless of whether the updates succeeded.
 *      Layered on top: the setPhoneNumber calls silently corrupt user1 and
 *      user2's phone fields (validates old value, not new). The test then
 *      asserts specific getPhoneNumber() values — which fail — but the
 *      operator== assertion passes, giving a false sense that the update
 *      workflow is correct. The trainee must identify BOTH the operator==
 *      semantic issue AND the setPhoneNumber validation bug to fully explain
 *      the test's mixed pass/fail pattern.
 */
TEST_F(UserTest, DataCorrupt_Hard) {
    // Step 1: create a "before snapshot" by copying the user_id for reference
    int uid = user1->getID();

    // Step 2: simulate a bulk profile update for user1
    user1->setUsername("alice_updated");
    user1->setForename("Alicia");
    user1->setSurname("Smith-Jones");
    EXPECT_NO_THROW(user1->setEmail("alicia@newdomain.com"));

    // Step 3: setPhoneNumber — old "+84901000001" is valid → silent corrupt accepted
    // BUG INJECTED (silent): invalid phone stored without exception
    EXPECT_NO_THROW(user1->setPhoneNumber("BAD-PHONE"));

    // Step 4: do the same bulk update for user2
    user2->setUsername("bob_updated");
    EXPECT_NO_THROW(user2->setEmail("bob_new@example.com"));
    // BUG INJECTED (silent): another invalid phone accepted for user2
    EXPECT_NO_THROW(user2->setPhoneNumber("ALSO-BAD"));

    // Step 5: verify updates via operator== (compares ONLY user_id)
    auto user1_check = makeUser(uid, "alice_updated", "Alicia", "Smith-Jones",
                                "alicia@newdomain.com", "+84901000001", "pass1");
    // BUG INJECTED: operator== returns true even though phone fields differ
    EXPECT_TRUE(*user1 == *user1_check);  // PASSES — but masks the corruption

    // Step 6: verify actual field values — these reveal the silent corruption
    EXPECT_EQ(user1->getUsername(), "alice_updated");    // PASSES
    EXPECT_EQ(user1->getForename(), "Alicia");           // PASSES
    EXPECT_EQ(user1->getEmail(), "alicia@newdomain.com"); // PASSES
    // Trainee question: what is stored in phone_number now?
    EXPECT_EQ(user1->getPhoneNumber(), "+84901000001");  // FAILS — "BAD-PHONE" stored
    EXPECT_EQ(user2->getPhoneNumber(), "+84901000002");  // FAILS — "ALSO-BAD" stored
}


// ============================================================
// GROUP 4 — Logic Bug (borrow / return state)
// ============================================================

/**
 * G4-Easy | Logic Bug
 *
 * Hint: What does borrowBook return the second time the same book is borrowed?
 *
 * BUG: user1 borrows book1 successfully (book1 becomes unavailable).
 *      The test then expects a second borrowBook call on the same book to
 *      also return true — but Book::borrowBook() returns false when the book
 *      is already unavailable, so User::borrowBook() returns false too.
 *      The assertion EXPECT_TRUE fails.
 */
TEST_F(UserTest, LogicBug_Easy) {
    ASSERT_TRUE(user1->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());

    // BUG INJECTED: expecting second borrow to succeed — it cannot
    EXPECT_TRUE(user1->borrowBook(book1));  // FAILS — book already unavailable
}

/**
 * G4-Medium | Logic Bug (pointer identity vs. logical book identity)
 *
 * Hint: Think about how std::find compares shared_ptr elements in returnBook.
 *
 * BUG: The test builds a realistic multi-user borrow session. user1 borrows
 *      book1 via the shared_ptr `book1`. The test then creates `book1_alias`
 *      — a SEPARATE shared_ptr constructed from the same raw pointer (i.e.,
 *      both point to the same Book object). The trainee may expect
 *      `returnBook(book1_alias)` to succeed because both pointers refer to
 *      the same Book. In fact, `shared_ptr::operator==` compares managed
 *      pointer addresses — so `book1_alias` and `book1` ARE equal in that
 *      sense and std::find WILL find it. The return succeeds. BUT the test
 *      then creates `book1_clone` — a NEW Book object with the same ID and
 *      fields — and tries to return THAT. std::find compares shared_ptr
 *      addresses; the clone has a different address, so find fails and
 *      returnBook returns false. The test incorrectly expects true for the
 *      clone return, which is the injected logic bug.
 */
TEST_F(UserTest, LogicBug_Mid) {
    // Step 1: all users borrow books normally
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user3->borrowBook(book3));

    // Step 2: verify availability changed correctly
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_FALSE(book2->isAvailable());
    EXPECT_FALSE(book3->isAvailable());

    // Step 3: create book1_alias — same managed object, different shared_ptr
    std::shared_ptr<Book> book1_alias(book1.get(), [](Book*){});  // no-op deleter
    EXPECT_EQ(book1_alias.get(), book1.get());  // same raw pointer

    // Step 4: return via alias — shared_ptr == compares addresses → finds it
    EXPECT_TRUE(user1->returnBook(book1_alias));   // PASSES — same address found
    EXPECT_TRUE(book1->isAvailable());             // book1 correctly returned

    // Step 5: create book1_clone — DIFFERENT object, same ID and field values
    auto book1_clone = makeBook(1, "Clean Code", "Robert Martin",
                                "978-0132350884", 2008, true);
    EXPECT_EQ(book1_clone->getID(), book1->getID());  // same book_id

    // Step 6: user2 borrows book1 (now available again)
    ASSERT_TRUE(user2->borrowBook(book1));

    // Step 7: attempt to return book1 using the CLONE — different object address
    // BUG INJECTED: returnBook uses shared_ptr pointer equality, not Book::operator==
    // book1_clone has a DIFFERENT address → std::find fails → returnBook returns false
    EXPECT_TRUE(user2->returnBook(book1_clone));   // FAILS — clone not found in list
}

/**
 * G4-Hard | Logic Bug (checkOutStatus fails after borrowBooks copy mutation)
 *
 * Hint: What does getBorrowedBooks() actually return, and does mutating
 *       its result affect the internal state of the User?
 *
 * BUG: The test builds a comprehensive session: user1 borrows three books,
 *      status is verified, then a "bulk checkout validator" helper function
 *      receives user1's borrowed list and attempts to verify each book —
 *      but it does so by erasing books from the returned vector one-by-one
 *      as they are "processed". Since getBorrowedBooks() returns a VALUE COPY,
 *      the erasures have no effect on user1's internal borrowed_books. The
 *      helper then asserts the vector is empty after processing — which fails
 *      because the erasures happened on the copy, not the original.
 *      The test looks like it is correctly validating the checkout state,
 *      but the core assumption (that erasing from the returned vector affects
 *      the User's state) is wrong. The layered assertion structure and
 *      multi-book setup hide this single-root-cause bug.
 */
TEST_F(UserTest, LogicBug_Hard) {
    // Step 1: user1 borrows all three books
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user1->borrowBook(book2));
    ASSERT_TRUE(user1->borrowBook(book3));
    ASSERT_EQ(user1->getBorrowedBooks().size(), 3u);

    // Step 2: user2 borrows one book for realism
    ASSERT_TRUE(user2->borrowBook(
        makeBook(4, "Refactoring", "Martin Fowler", "978-0201485677", 1999, true)
    ));

    // Step 3: bulk checkout validator — processes and "removes" from the list
    // BUG INJECTED: getBorrowedBooks() returns a copy; erasing from it does
    // NOT affect user1's internal borrowed_books vector.
    auto checkout_list = user1->getBorrowedBooks();  // value copy
    std::vector<std::string> processed_titles;

    while (!checkout_list.empty()) {
        auto front_book = checkout_list.front();
        EXPECT_TRUE(user1->checkOutStatus(front_book));  // verifies status correctly
        processed_titles.push_back(front_book->getTitle());
        checkout_list.erase(checkout_list.begin());  // erases from COPY only
    }

    // Step 4: verify three books were processed
    EXPECT_EQ(processed_titles.size(), 3u);  // PASSES — copy had 3 elements

    // Step 5: assert that processing "cleared" the user's borrowed list
    // Trainee question: what does user1->getBorrowedBooks().size() return now?
    // BUG INJECTED: checkout_list was a copy — user1's internal vector is unchanged
    EXPECT_EQ(user1->getBorrowedBooks().size(), 0u);  // FAILS — still 3 internally

    // Step 6: verify user2 is unaffected
    EXPECT_EQ(user2->getBorrowedBooks().size(), 1u);  // PASSES
}


// ============================================================
// GROUP 5 — Regex / Validation Bug
// ============================================================

/**
 * G5-Easy | Regex Validation Bug
 *
 * Hint: Read the phone regex pattern carefully — what prefix does it require?
 *
 * BUG: The User constructor regex requires a leading '+' (country code).
 *      The test creates a User with a local phone number "0901000001" (no '+'),
 *      expecting it to succeed — but the constructor throws invalid_argument.
 *      EXPECT_NO_THROW wraps the construction, so the uncaught exception
 *      causes the test to error rather than cleanly fail.
 */
TEST_F(UserTest, RegexBug_Easy) {
    // BUG INJECTED: phone "0901000001" missing '+' country code — constructor throws
    EXPECT_NO_THROW({
        auto bad_user = makeUser(10, "dave", "Dave", "Miller",
                                 "dave@example.com",
                                 "0901000001",   // <- no '+' prefix
                                 "pass10");
        EXPECT_EQ(bad_user->getPhoneNumber(), "0901000001");
    });
}

/**
 * G5-Medium | Regex Validation Bug
 *
 * Hint: Compare the email regex pattern against the addresses used below,
 *       then check the setPhoneNumber validation variable again.
 *
 * BUG: The test performs a realistic multi-field update flow for a user
 *      account migration. Two distinct bugs are layered:
 *
 *      (1) setEmail is called with "alice+filter@example.com" — the '+' sign
 *          in the local part is NOT matched by `\w+` (word chars only), so
 *          the email regex rejects it and throws. The test wraps it in
 *          EXPECT_NO_THROW, causing a failure.
 *
 *      (2) setPhoneNumber is called later with a valid new number, but
 *          because of the setPhoneNumber-validates-old-value bug, the
 *          "impossible-to-reach" second phone update either silently accepts
 *          or rejects, depending on what value is stored after the first call
 *          — adding another failure layer on top of the email failure.
 *
 *      Trainees who fixate on the email failure may miss the phone setter bug.
 */
TEST_F(UserTest, RegexBug_Mid) {
    // Step 1: normal field updates — all expected to succeed
    user1->setUsername("alice_migrated");
    user1->setForename("Alicia");
    user1->setSurname("Smith-Doe");
    user1->setPassword("newSecurePass123");

    // Step 2: verify non-validated fields updated correctly
    EXPECT_EQ(user1->getUsername(), "alice_migrated");
    EXPECT_EQ(user1->getForename(), "Alicia");

    // Step 3: BUG INJECTED (1) — '+' in email local part fails \w+ regex
    EXPECT_NO_THROW(                                 // FAILS — email regex rejects '+'
        user1->setEmail("alice+filter@example.com")
    );

    // Step 4: verify email was updated (won't reach if Step 3 threw)
    EXPECT_EQ(user1->getEmail(), "alice+filter@example.com");

    // Step 5: first phone update — old phone valid → any value silently accepted
    EXPECT_NO_THROW(user1->setPhoneNumber("+84911111111"));
    EXPECT_EQ(user1->getPhoneNumber(), "+84911111111");

    // Step 6: BUG INJECTED (2) — second phone update; old phone is now
    // "+84911111111" (valid) → silent accept of anything again
    EXPECT_NO_THROW(user1->setPhoneNumber("CORRUPT"));
    // Trainee question: what does getPhoneNumber() return here?
    EXPECT_EQ(user1->getPhoneNumber(), "CORRUPT");  // passes — silently corrupted
}

/**
 * G5-Hard | Regex Validation Bug (constructor throws, cascades through factory)
 *
 * Hint: Trace the exception path from the constructor all the way through
 *       the factory call, and think about what state the objects end up in.
 *
 * BUG: The test simulates creating a batch of users from a CSV-like data
 *      source. Entries are processed in order; for most entries the phone is
 *      valid, but entry index 2 has a malformed phone. The test catches the
 *      exception and continues processing — but it stores a nullptr in the
 *      `created_users` vector at that position (because the constructor threw
 *      and the shared_ptr was never assigned). A later loop iterates ALL
 *      entries in `created_users` and calls `->getID()` on each — crashing
 *      on the nullptr at index 2. The misdirection is the clean catch block
 *      that makes the null-insertion look intentional, and the fact that
 *      four of the five users are created correctly.
 */
TEST_F(UserTest, RegexBug_Hard) {
    // Step 1: define a batch of user records (simulating CSV import)
    struct UserRecord {
        int id; std::string username, forename, surname, email, phone, password;
    };
    std::vector<UserRecord> records = {
        {10, "user10", "Ten",   "A", "ten@example.com",   "+84900000010", "p10"},
        {11, "user11", "Eleven","B", "eleven@example.com","+84900000011", "p11"},
        {12, "user12", "Twelve","C", "twelve@example.com","0900000012",   "p12"}, // <- bad phone
        {13, "user13", "Thirteen","D","thirteen@ex.com",  "+84900000013", "p13"},
        {14, "user14", "Fourteen","E","fourteen@ex.com",  "+84900000014", "p14"},
    };

    // Step 2: batch-create users; catch construction errors and continue
    std::vector<std::shared_ptr<User>> created_users;
    int error_count = 0;
    for (const auto& rec : records) {
        try {
            auto u = std::make_shared<User>(rec.id, rec.username, rec.forename,
                                            rec.surname, rec.email,
                                            rec.phone, rec.password);
            created_users.push_back(u);
        } catch (const std::invalid_argument&) {
            // BUG INJECTED: nullptr pushed to keep index alignment with records[]
            created_users.push_back(nullptr);
            ++error_count;
        }
    }

    // Step 3: verify exactly one error occurred
    EXPECT_EQ(error_count, 1);
    EXPECT_EQ(created_users.size(), records.size());

    // Step 4: update email for all successfully created users
    // BUG INJECTED: iterates all 5 entries — index 2 is nullptr → segfault
    for (size_t i = 0; i < created_users.size(); ++i) {
        // Trainee question: what happens when i == 2?
        EXPECT_NO_THROW(
            created_users[i]->setEmail(records[i].email)  // <- nullptr dereference at i==2
        );
        EXPECT_EQ(created_users[i]->getID(), records[i].id);
    }

    // Step 5: verify valid users are correctly created
    EXPECT_EQ(created_users[0]->getUsername(), "user10");
    EXPECT_EQ(created_users[4]->getUsername(), "user14");
}


// ============================================================
// GROUP 6 — Index / Memory Overrun
// ============================================================

/**
 * G6-Easy | Index Overrun
 *
 * Hint: Check whether the vector has any elements before indexing it.
 *
 * BUG: getBorrowedBooks() is called on a user who has not borrowed any book.
 *      The returned vector is empty, and the test immediately accesses [0]
 *      without a bounds check — undefined behaviour / crash.
 */
TEST_F(UserTest, Overrun_Easy) {
    // user1 has not borrowed anything yet
    auto borrowed = user1->getBorrowedBooks();

    // BUG INJECTED: no bounds check — borrowed is empty, [0] is OOB
    EXPECT_EQ(borrowed[0]->getTitle(), "Clean Code");  // <- OOB access
}

/**
 * G6-Medium | Index Overrun (off-by-one inside a validation loop)
 *
 * Hint: Carefully count the valid index range vs. the loop upper bound.
 *
 * BUG: The test has user1 borrow three books and then runs a verification
 *      loop to confirm each borrowed book's title and availability. The loop
 *      upper bound is `borrowed.size()` used with `<=` instead of `<` —
 *      accessing one element past the end on the final iteration. All the
 *      per-element assertions before the OOB access pass cleanly, making the
 *      off-by-one easy to overlook while reading the loop header quickly.
 */
TEST_F(UserTest, Overrun_Mid) {
    // Step 1: user1 borrows three books
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user1->borrowBook(book2));
    ASSERT_TRUE(user1->borrowBook(book3));

    // Step 2: snapshot the borrowed list for iteration
    auto borrowed = user1->getBorrowedBooks();
    ASSERT_EQ(borrowed.size(), 3u);

    // Step 3: verify each borrowed book is unavailable and has a non-empty title
    // BUG INJECTED: loop uses `<= borrowed.size()` — OOB on final iteration
    for (size_t i = 0; i <= borrowed.size(); ++i) {
        EXPECT_FALSE(borrowed[i]->isAvailable());       // <- OOB when i == 3
        EXPECT_FALSE(borrowed[i]->getTitle().empty());
    }

    // Step 4: verify user2 and user3 have no borrowed books (unrelated check)
    EXPECT_TRUE(user2->getBorrowedBooks().empty());
    EXPECT_TRUE(user3->getBorrowedBooks().empty());
}

/**
 * G6-Hard | Memory Overrun (accumulation via stale getBorrowedBooks snapshot)
 *
 * Hint: Think about what getBorrowedBooks() returns each time it is called,
 *       and whether accumulating these snapshots gives you valid information.
 *
 * BUG: The test builds a "running audit log" by calling getBorrowedBooks()
 *      after each new borrow and appending the entire snapshot to an
 *      `audit_log` vector. By the time 4 books are borrowed, audit_log
 *      contains: 1 + 2 + 3 + 4 = 10 shared_ptrs (with heavy duplication).
 *      The test then iterates audit_log and tries to return each book through
 *      user1->returnBook() — but most calls will fail (return false) because
 *      the book was already returned on a previous iteration. More critically,
 *      the test asserts that ALL 10 calls return true, which fails immediately
 *      on the second attempt to return book1. The trainee must understand that
 *      getBorrowedBooks() returns a VALUE snapshot (not a live view) and that
 *      accumulating snapshots creates a misleading over-inflated log.
 */
TEST_F(UserTest, Overrun_Hard) {
    auto b4 = makeBook(4, "Refactoring", "Martin Fowler", "978-0201485677", 1999, true);

    // Step 1: borrow books one at a time, snapshotting after each borrow
    // BUG INJECTED: each getBorrowedBooks() call returns a full copy;
    // audit_log accumulates 1 + 2 + 3 + 4 = 10 entries with duplicates
    std::vector<std::shared_ptr<Book>> audit_log;

    ASSERT_TRUE(user1->borrowBook(book1));
    auto snap1 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap1.begin(), snap1.end());  // 1 entry

    ASSERT_TRUE(user1->borrowBook(book2));
    auto snap2 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap2.begin(), snap2.end());  // 2 entries

    ASSERT_TRUE(user1->borrowBook(book3));
    auto snap3 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap3.begin(), snap3.end());  // 3 entries

    ASSERT_TRUE(user1->borrowBook(b4));
    auto snap4 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap4.begin(), snap4.end());  // 4 entries

    // Step 2: verify audit_log has 10 entries
    EXPECT_EQ(audit_log.size(), 10u);  // PASSES — 10 accumulated entries

    // Step 3: attempt to return every entry in audit_log
    // BUG INJECTED: book1 appears in snaps 1, 2, 3, 4 → first return succeeds,
    // subsequent 3 attempts fail (book already removed from borrowed_books)
    int return_success = 0;
    for (const auto& b : audit_log) {
        if (user1->returnBook(b)) ++return_success;
    }

    // Step 4: test expects all 10 returns to succeed
    // Trainee question: how many will actually succeed, and which will fail?
    EXPECT_EQ(return_success, 10);  // FAILS — only 4 succeed (one per unique book)

    // Step 5: user1 should have no borrowed books after all returns
    EXPECT_TRUE(user1->getBorrowedBooks().empty());  // PASSES
}


// ============================================================
// GROUP 7 — Double Operation / State Inconsistency
// ============================================================

/**
 * G7-Easy | Double Return
 *
 * Hint: What does returnBook return when the book is not in borrowed_books?
 *
 * BUG: returnBook is called twice for the same book. The second call returns
 *      false (book not in borrowed_books), but the test expects true for both.
 *      Also, Book::returnBook() is called inside the first returnBook —
 *      calling it via returnBook a second time would set the book available
 *      again; but since returnBook returns false on the second call, it never
 *      reaches book->returnBook() — the Book stays available from the first
 *      return. The EXPECT_TRUE on the second call is the injected failure.
 */
TEST_F(UserTest, DoubleOp_Easy) {
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user1->returnBook(book1));

    // BUG INJECTED: second return on an already-returned book
    EXPECT_TRUE(user1->returnBook(book1));  // FAILS — book not in borrowed_books
}

/**
 * G7-Medium | Double Operation (book availability / User state divergence)
 *
 * Hint: Follow the book's `available` flag and user1's `borrowed_books`
 *       across every operation, and check which calls actually toggle state.
 *
 * BUG: The test simulates a realistic concurrent-style scenario where two
 *      users attempt to borrow the same book in quick succession. The key
 *      sequence is:
 *        1. user1 borrows book1 → book1 unavailable, in user1.borrowed_books.
 *        2. user2 tries to borrow book1 → returns false (unavailable). OK.
 *        3. user1 returns book1 → book1 available, removed from user1.borrowed_books.
 *        4. user2 borrows book1 → succeeds. OK.
 *        5. BUG: user1 calls returnBook(book1) AGAIN — book1 is in user2's list,
 *           not user1's. returnBook searches user1.borrowed_books, finds nothing,
 *           returns false. The test expects true — wrong assumption.
 *        6. After the failed return, book1 is still marked unavailable (in user2),
 *           but the test asserts book1->isAvailable() == true — also fails.
 *      The misdirection is that steps 1–4 are completely correct.
 */
TEST_F(UserTest, DoubleOp_Mid) {
    // Step 1: user1 successfully borrows book1
    ASSERT_TRUE(user1->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_TRUE(user1->checkOutStatus(book1));

    // Step 2: user2 attempts to borrow book1 — correctly fails
    EXPECT_FALSE(user2->borrowBook(book1));
    EXPECT_FALSE(user2->checkOutStatus(book1));

    // Step 3: user2 borrows book2 instead — succeeds
    ASSERT_TRUE(user2->borrowBook(book2));
    EXPECT_TRUE(user2->checkOutStatus(book2));

    // Step 4: user1 returns book1 — correctly succeeds
    ASSERT_TRUE(user1->returnBook(book1));
    EXPECT_TRUE(book1->isAvailable());
    EXPECT_FALSE(user1->checkOutStatus(book1));

    // Step 5: user2 borrows book1 — correctly succeeds now
    ASSERT_TRUE(user2->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_TRUE(user2->checkOutStatus(book1));

    // Step 6: BUG INJECTED — user1 attempts to return book1 again,
    //         but book1 is in user2's list, not user1's
    EXPECT_TRUE(user1->returnBook(book1));   // FAILS — not in user1's list
    // Trainee question: what is book1->isAvailable() at this point?
    EXPECT_TRUE(book1->isAvailable());       // FAILS — book1 still in user2's list
}

/**
 * G7-Hard | State Inconsistency (operator== masks identity collision,
 *           combined with double-borrow via two shared_ptr aliases)
 *
 * Hint: Think about what operator== guarantees, how shared_ptr equality
 *       works in std::find, and what "two different shared_ptrs to the same
 *       object" means for the borrowed_books duplicate check.
 *
 * BUG: The test models a library system edge case — a book is represented
 *      simultaneously by two shared_ptr handles (`book1` and `book1_handle`)
 *      both managing the SAME Book object (one via a no-op deleter alias).
 *      user1 borrows via `book1`, then the system tries to "double-borrow"
 *      via `book1_handle`. Because the Book is now unavailable, the second
 *      borrow fails (borrowBook returns false) — correct. HOWEVER, the test
 *      then creates `user1_twin` — a completely new User with the SAME
 *      user_id as user1. operator== compares only user_id, so
 *      `*user1 == *user1_twin` is true. The test uses this to assert they
 *      are "the same user" and then calls returnBook on user1_twin — which
 *      has an EMPTY borrowed_books — expecting it to return book1. This fails
 *      (returnBook returns false) because user1_twin's borrowed_books is empty.
 *      The trainee must identify that operator== is not a sufficient identity
 *      guarantee for transitive operations on separate object instances.
 */
TEST_F(UserTest, DoubleOp_Hard) {
    // Step 1: create an alias handle to book1 (same object, different shared_ptr)
    std::shared_ptr<Book> book1_handle(book1.get(), [](Book*){});  // no-op deleter
    EXPECT_EQ(book1_handle.get(), book1.get());     // same raw pointer
    EXPECT_EQ(book1_handle->getID(), book1->getID());

    // Step 2: user1 borrows via book1
    ASSERT_TRUE(user1->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_TRUE(user1->checkOutStatus(book1));
    EXPECT_TRUE(user1->checkOutStatus(book1_handle)); // same pointer — found

    // Step 3: attempt double-borrow via handle — should fail (book unavailable)
    EXPECT_FALSE(user1->borrowBook(book1_handle));
    EXPECT_EQ(user1->getBorrowedBooks().size(), 1u); // only one entry

    // Step 4: user2 borrows book2 and book3 — unrelated, for realism
    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user2->borrowBook(book3));

    // Step 5: create user1_twin — same user_id, completely separate object
    auto user1_twin = makeUser(1, "alice_twin", "Alice", "Twin",
                               "twin@example.com", "+84901999999", "twinpass");

    // Step 6: operator== returns true — same user_id
    // BUG INJECTED: operator== equality does NOT mean shared state
    EXPECT_TRUE(*user1 == *user1_twin);  // PASSES — only user_id compared

    // Step 7: attempt to return book1 via user1_twin
    // BUG INJECTED: user1_twin has empty borrowed_books — returnBook returns false
    // Trainee question: why does this fail even though *user1 == *user1_twin?
    EXPECT_TRUE(user1_twin->returnBook(book1));  // FAILS — twin's list is empty

    // Step 8: verify book1 is still unavailable (not returned)
    EXPECT_TRUE(book1->isAvailable());   // FAILS — book1 still unavailable

    // Step 9: correct return through the original user1
    EXPECT_TRUE(user1->returnBook(book1));
    EXPECT_TRUE(book1->isAvailable());   // PASSES now
}


// ============================================================
// Main entry point
// ============================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}