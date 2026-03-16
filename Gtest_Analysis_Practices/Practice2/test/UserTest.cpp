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
    const std::string& username = "alice",
    const std::string& forename = "Alice",
    const std::string& surname = "Smith",
    const std::string& email = "alice@example.com",
    const std::string& phone = "+84901000001",
    const std::string& password = "pass1")
{
    return std::make_shared<User>(id, username, forename, surname, email, phone, password);
}

static std::shared_ptr<Book> makeBook(
    int id,
    const std::string& title = "Clean Code",
    const std::string& author = "Robert Martin",
    const std::string& isbn = "978-0132350884",
    int year = 2008,
    bool available = true)
{
    return std::make_shared<Book>(id, title, author, isbn, year, available);
}

class UserTest : public ::testing::Test {
protected:
    std::shared_ptr<User> user1;
    std::shared_ptr<User> user2;
    std::shared_ptr<User> user3;
    std::shared_ptr<Book> book1;
    std::shared_ptr<Book> book2;
    std::shared_ptr<Book> book3;

    void SetUp() override {
        user1 = makeUser(1, "alice", "Alice", "Smith", "alice@example.com", "+84901000001", "pass1");
        user2 = makeUser(2, "bob", "Bob", "Jones", "bob@example.com", "+84901000002", "pass2");
        user3 = makeUser(3, "charlie", "Charlie", "Brown", "charlie@example.com", "+84901000003", "pass3");
        book1 = makeBook(1, "Clean Code", "Robert Martin", "978-0132350884", 2008, true);
        book2 = makeBook(2, "The Pragmatic Programmer", "David Thomas", "978-0201616224", 1999, true);
        book3 = makeBook(3, "Design Patterns", "Gang of Four", "978-0201633610", 1994, true);
    }
};

/* ──────────────────────────────────────────────────────────────────────────
   GROUP 1 — Segmentation Fault
────────────────────────────────────────────────────────────────────────── */

TEST_F(UserTest, SegFault_Easy) {
    std::shared_ptr<Book> null_book = nullptr;
    EXPECT_TRUE(user1->borrowBook(null_book));
}

TEST_F(UserTest, SegFault_Mid) {
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user3->borrowBook(book3));

    EXPECT_EQ(user1->getBorrowedBooks().size(), 1u);
    EXPECT_EQ(user2->getBorrowedBooks().size(), 1u);
    EXPECT_EQ(user3->getBorrowedBooks().size(), 1u);

    auto borrowed = user1->getBorrowedBooks();
    ASSERT_FALSE(borrowed.empty());

    const Book* raw_ptr = borrowed.front().get();

    ASSERT_TRUE(user1->returnBook(book1));

    borrowed.clear();

    EXPECT_EQ(raw_ptr->getTitle(), "Clean Code");
}

TEST_F(UserTest, SegFault_Hard) {
    ASSERT_TRUE(user1->borrowBook(book1));

    auto first_copy = user1->getBorrowedBooks();
    ASSERT_EQ(first_copy.size(), 1u);
    const Book* stable_ptr = first_copy.front().get();
    std::string expected_title = first_copy.front()->getTitle();

    std::vector<std::shared_ptr<Book>> extra_books;
    for (int i = 10; i < 40; ++i) {
        auto b = makeBook(i, "Extra Book " + std::to_string(i),
                          "Author " + std::to_string(i),
                          "ISBN-" + std::to_string(i), 2000 + i, true);
        extra_books.push_back(b);
        ASSERT_TRUE(user1->borrowBook(b));
    }

    auto second_copy = user1->getBorrowedBooks();
    ASSERT_GE(second_copy.size(), 31u);
    first_copy.clear();

    second_copy.clear();

    EXPECT_EQ(stable_ptr->getTitle(), expected_title);
}

/* ──────────────────────────────────────────────────────────────────────────
   GROUP 2 — Use After Free
────────────────────────────────────────────────────────────────────────── */

TEST_F(UserTest, UseAfterFree_Easy) {
    user1->borrowBook(book1);
    const std::shared_ptr<Book>& book_ref = user1->getBorrowedBooks().front();
    EXPECT_EQ(book_ref->getTitle(), "Clean Code");
}

TEST_F(UserTest, UseAfterFree_Mid) {
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user3->borrowBook(book3));

    EXPECT_TRUE(user1->checkOutStatus(book1));
    EXPECT_TRUE(user2->checkOutStatus(book2));
    EXPECT_TRUE(user3->checkOutStatus(book3));

    auto snapshot = user1->getBorrowedBooks();
    ASSERT_EQ(snapshot.size(), 1u);

    const Book* raw_after_reset = snapshot[0].get();
    EXPECT_EQ(raw_after_reset->getTitle(), "Clean Code");

    ASSERT_TRUE(user1->returnBook(book1));
    EXPECT_FALSE(user1->checkOutStatus(book1));

    ASSERT_TRUE(user2->borrowBook(book3));
    EXPECT_EQ(user2->getBorrowedBooks().size(), 2u);

    snapshot[0].reset();
    book1.reset();

    EXPECT_EQ(raw_after_reset->getTitle(), "Clean Code");
}

TEST_F(UserTest, UseAfterFree_Hard) {
    std::vector<std::shared_ptr<Book>> all_books = {book1, book2, book3};
    auto b4 = makeBook(4, "Refactoring", "Martin Fowler", "978-0201485677", 1999, true);
    auto b5 = makeBook(5, "Clean Architecture", "Robert Martin", "978-0134494166", 2017, true);
    all_books.push_back(b4);
    all_books.push_back(b5);

    for (auto& b : all_books) {
        ASSERT_TRUE(user1->borrowBook(b));
    }
    ASSERT_EQ(user1->getBorrowedBooks().size(), 5u);

    auto snapshot = user1->getBorrowedBooks();

    std::vector<std::shared_ptr<Book>> live_view;
    for (size_t i = 0; i < snapshot.size(); ++i) {
        live_view = user1->getBorrowedBooks();
        bool returned = user1->returnBook(snapshot[i]);
        EXPECT_TRUE(returned);
    }

    for (const auto& b : all_books) {
        EXPECT_FALSE(user1->checkOutStatus(b));
    }

    EXPECT_TRUE(live_view.empty());
    EXPECT_EQ(user1->getBorrowedBooks().size(), 0u);
}

/* ──────────────────────────────────────────────────────────────────────────
   GROUP 3 — Data Corruption
────────────────────────────────────────────────────────────────────────── */

TEST_F(UserTest, DataCorrupt_Easy) {
    EXPECT_THROW(
        user1->setPhoneNumber("not-a-phone-number"),
        std::invalid_argument
    );
}

TEST_F(UserTest, DataCorrupt_Mid) {
    EXPECT_EQ(user1->getPhoneNumber(), "+84901000001");
    EXPECT_EQ(user2->getPhoneNumber(), "+84901000002");
    EXPECT_EQ(user3->getPhoneNumber(), "+84901000003");

    EXPECT_NO_THROW(user1->setPhoneNumber("+00"));

    EXPECT_NO_THROW(user1->setPhoneNumber("+84999999991"));

    EXPECT_NO_THROW(user2->setPhoneNumber("INVALID-PHONE"));
    EXPECT_EQ(user2->getPhoneNumber(), "INVALID-PHONE");

    EXPECT_NO_THROW(user3->setPhoneNumber("+84911111111"));
    EXPECT_EQ(user3->getPhoneNumber(), "+84911111111");

    EXPECT_EQ(user1->getPhoneNumber(), "+84999999991");
    EXPECT_EQ(user2->getPhoneNumber(), "+84901000002");
}

TEST_F(UserTest, DataCorrupt_Hard) {
    int uid = user1->getID();

    user1->setUsername("alice_updated");
    user1->setForename("Alicia");
    user1->setSurname("Smith-Jones");
    EXPECT_NO_THROW(user1->setEmail("alicia@newdomain.com"));

    EXPECT_NO_THROW(user1->setPhoneNumber("BAD-PHONE"));

    user2->setUsername("bob_updated");
    EXPECT_NO_THROW(user2->setEmail("bob_new@example.com"));
    EXPECT_NO_THROW(user2->setPhoneNumber("ALSO-BAD"));

    auto user1_check = makeUser(uid, "alice_updated", "Alicia", "Smith-Jones",
                                "alicia@newdomain.com", "+84901000001", "pass1");

    EXPECT_TRUE(*user1 == *user1_check);

    EXPECT_EQ(user1->getUsername(), "alice_updated");
    EXPECT_EQ(user1->getForename(), "Alicia");
    EXPECT_EQ(user1->getEmail(), "alicia@newdomain.com");

    EXPECT_EQ(user1->getPhoneNumber(), "+84901000001");
    EXPECT_EQ(user2->getPhoneNumber(), "+84901000002");
}

/* ──────────────────────────────────────────────────────────────────────────
   GROUP 4 — Logic Bug
────────────────────────────────────────────────────────────────────────── */

TEST_F(UserTest, LogicBug_Easy) {
    ASSERT_TRUE(user1->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());

    EXPECT_TRUE(user1->borrowBook(book1));
}

TEST_F(UserTest, LogicBug_Mid) {
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user3->borrowBook(book3));

    EXPECT_FALSE(book1->isAvailable());
    EXPECT_FALSE(book2->isAvailable());
    EXPECT_FALSE(book3->isAvailable());

    std::shared_ptr<Book> book1_alias(book1.get(), [](Book*){});
    EXPECT_EQ(book1_alias.get(), book1.get());

    EXPECT_TRUE(user1->returnBook(book1_alias));
    EXPECT_TRUE(book1->isAvailable());

    auto book1_clone = makeBook(1, "Clean Code", "Robert Martin",
                                "978-0132350884", 2008, true);
    EXPECT_EQ(book1_clone->getID(), book1->getID());

    ASSERT_TRUE(user2->borrowBook(book1));

    EXPECT_TRUE(user2->returnBook(book1_clone));
}

TEST_F(UserTest, LogicBug_Hard) {
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user1->borrowBook(book2));
    ASSERT_TRUE(user1->borrowBook(book3));
    ASSERT_EQ(user1->getBorrowedBooks().size(), 3u);

    ASSERT_TRUE(user2->borrowBook(
        makeBook(4, "Refactoring", "Martin Fowler", "978-0201485677", 1999, true)
    ));

    auto checkout_list = user1->getBorrowedBooks();
    std::vector<std::string> processed_titles;
    while (!checkout_list.empty()) {
        auto front_book = checkout_list.front();
        EXPECT_TRUE(user1->checkOutStatus(front_book));
        processed_titles.push_back(front_book->getTitle());
        checkout_list.erase(checkout_list.begin());
    }

    EXPECT_EQ(processed_titles.size(), 3u);

    EXPECT_EQ(user1->getBorrowedBooks().size(), 0u);

    EXPECT_EQ(user2->getBorrowedBooks().size(), 1u);
}

/* ──────────────────────────────────────────────────────────────────────────
   GROUP 5 — Regex / Validation Bug
────────────────────────────────────────────────────────────────────────── */

TEST_F(UserTest, RegexBug_Easy) {
    EXPECT_NO_THROW({
        auto bad_user = makeUser(10, "dave", "Dave", "Miller",
                                 "dave@example.com",
                                 "0901000001",
                                 "pass10");
        EXPECT_EQ(bad_user->getPhoneNumber(), "0901000001");
    });
}

TEST_F(UserTest, RegexBug_Mid) {
    user1->setUsername("alice_migrated");
    user1->setForename("Alicia");
    user1->setSurname("Smith-Doe");
    user1->setPassword("newSecurePass123");

    EXPECT_EQ(user1->getUsername(), "alice_migrated");
    EXPECT_EQ(user1->getForename(), "Alicia");

    EXPECT_NO_THROW(
        user1->setEmail("alice+filter@example.com")
    );

    EXPECT_EQ(user1->getEmail(), "alice+filter@example.com");

    EXPECT_NO_THROW(user1->setPhoneNumber("+84911111111"));
    EXPECT_EQ(user1->getPhoneNumber(), "+84911111111");

    EXPECT_NO_THROW(user1->setPhoneNumber("CORRUPT"));

    EXPECT_EQ(user1->getPhoneNumber(), "CORRUPT");
}

TEST_F(UserTest, RegexBug_Hard) {
    struct UserRecord {
        int id; std::string username, forename, surname, email, phone, password;
    };
    std::vector<UserRecord> records = {
        {10, "user10", "Ten", "A", "ten@example.com", "+84900000010", "p10"},
        {11, "user11", "Eleven","B", "eleven@example.com","+84900000011", "p11"},
        {12, "user12", "Twelve","C", "twelve@example.com","0900000012", "p12"},
        {13, "user13", "Thirteen","D","thirteen@ex.com", "+84900000013", "p13"},
        {14, "user14", "Fourteen","E","fourteen@ex.com", "+84900000014", "p14"},
    };

    std::vector<std::shared_ptr<User>> created_users;
    int error_count = 0;
    for (const auto& rec : records) {
        try {
            auto u = std::make_shared<User>(rec.id, rec.username, rec.forename,
                                            rec.surname, rec.email,
                                            rec.phone, rec.password);
            created_users.push_back(u);
        } catch (const std::invalid_argument&) {
            created_users.push_back(nullptr);
            ++error_count;
        }
    }

    EXPECT_EQ(error_count, 1);
    EXPECT_EQ(created_users.size(), records.size());

    for (size_t i = 0; i < created_users.size(); ++i) {
        EXPECT_NO_THROW(
            created_users[i]->setEmail(records[i].email)
        );
        EXPECT_EQ(created_users[i]->getID(), records[i].id);
    }

    EXPECT_EQ(created_users[0]->getUsername(), "user10");
    EXPECT_EQ(created_users[4]->getUsername(), "user14");
}

/* ──────────────────────────────────────────────────────────────────────────
   GROUP 6 — Index / Memory Overrun
────────────────────────────────────────────────────────────────────────── */

TEST_F(UserTest, Overrun_Easy) {
    auto borrowed = user1->getBorrowedBooks();
    EXPECT_EQ(borrowed[0]->getTitle(), "Clean Code");
}

TEST_F(UserTest, Overrun_Mid) {
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user1->borrowBook(book2));
    ASSERT_TRUE(user1->borrowBook(book3));

    auto borrowed = user1->getBorrowedBooks();
    ASSERT_EQ(borrowed.size(), 3u);

    for (size_t i = 0; i <= borrowed.size(); ++i) {
        EXPECT_FALSE(borrowed[i]->isAvailable());
        EXPECT_FALSE(borrowed[i]->getTitle().empty());
    }

    EXPECT_TRUE(user2->getBorrowedBooks().empty());
    EXPECT_TRUE(user3->getBorrowedBooks().empty());
}

TEST_F(UserTest, Overrun_Hard) {
    auto b4 = makeBook(4, "Refactoring", "Martin Fowler", "978-0201485677", 1999, true);

    std::vector<std::shared_ptr<Book>> audit_log;
    ASSERT_TRUE(user1->borrowBook(book1));
    auto snap1 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap1.begin(), snap1.end());

    ASSERT_TRUE(user1->borrowBook(book2));
    auto snap2 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap2.begin(), snap2.end());

    ASSERT_TRUE(user1->borrowBook(book3));
    auto snap3 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap3.begin(), snap3.end());

    ASSERT_TRUE(user1->borrowBook(b4));
    auto snap4 = user1->getBorrowedBooks();
    audit_log.insert(audit_log.end(), snap4.begin(), snap4.end());

    EXPECT_EQ(audit_log.size(), 10u);

    int return_success = 0;
    for (const auto& b : audit_log) {
        if (user1->returnBook(b)) ++return_success;
    }

    EXPECT_EQ(return_success, 10);

    EXPECT_TRUE(user1->getBorrowedBooks().empty());
}

/* ──────────────────────────────────────────────────────────────────────────
   GROUP 7 — Double Operation / State Inconsistency
────────────────────────────────────────────────────────────────────────── */

TEST_F(UserTest, DoubleOp_Easy) {
    ASSERT_TRUE(user1->borrowBook(book1));
    ASSERT_TRUE(user1->returnBook(book1));

    EXPECT_TRUE(user1->returnBook(book1));
}

TEST_F(UserTest, DoubleOp_Mid) {
    ASSERT_TRUE(user1->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_TRUE(user1->checkOutStatus(book1));

    EXPECT_FALSE(user2->borrowBook(book1));
    EXPECT_FALSE(user2->checkOutStatus(book1));

    ASSERT_TRUE(user2->borrowBook(book2));
    EXPECT_TRUE(user2->checkOutStatus(book2));

    ASSERT_TRUE(user1->returnBook(book1));
    EXPECT_TRUE(book1->isAvailable());
    EXPECT_FALSE(user1->checkOutStatus(book1));

    ASSERT_TRUE(user2->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_TRUE(user2->checkOutStatus(book1));

    EXPECT_TRUE(user1->returnBook(book1));

    EXPECT_TRUE(book1->isAvailable());
}

TEST_F(UserTest, DoubleOp_Hard) {
    std::shared_ptr<Book> book1_handle(book1.get(), [](Book*){});
    EXPECT_EQ(book1_handle.get(), book1.get());
    EXPECT_EQ(book1_handle->getID(), book1->getID());

    ASSERT_TRUE(user1->borrowBook(book1));
    EXPECT_FALSE(book1->isAvailable());
    EXPECT_TRUE(user1->checkOutStatus(book1));
    EXPECT_TRUE(user1->checkOutStatus(book1_handle));

    EXPECT_FALSE(user1->borrowBook(book1_handle));
    EXPECT_EQ(user1->getBorrowedBooks().size(), 1u);

    ASSERT_TRUE(user2->borrowBook(book2));
    ASSERT_TRUE(user2->borrowBook(book3));

    auto user1_twin = makeUser(1, "alice_twin", "Alice", "Twin",
                               "twin@example.com", "+84901999999", "twinpass");

    EXPECT_TRUE(*user1 == *user1_twin);

    EXPECT_TRUE(user1_twin->returnBook(book1));

    EXPECT_TRUE(book1->isAvailable());

    EXPECT_TRUE(user1->returnBook(book1));
    EXPECT_TRUE(book1->isAvailable());
}