<div align="center">
  
# Library Management System

This project is submitted in partial fulfilment of the Degree of **Master of Science in Computer Science**.

## Features

- **Book Management**:  
  - Add, remove, and search for books in the catalogue.  
  - Each book is represented as a `Book` class, managing attributes such as title, author, ISBN, year published, and availability status.
  - Methods include `getID()`, `getTitle()`, `getAuthor()`, `getISBN()`, `getYearPublished()`, `isAvailable()`, `setTitle()`, `setAuthor()`, `setISBN()`, `setYearPublished()`, `setIsAvailable()`, `borrowBook()`, `returnBook()`, and `getInfo()`.

- **User Management**:  
  - Add, remove, and manage library users.  
  - Each user is represented as a `User` class, managing attributes such as username, forename, surname, email, phone number, password, and borrowed books.
  - Methods include `getID()`, `getUsername()`, `getForename()`, `getSurname()`, `getEmail()`, `getPhoneNumber()`, `getPassword()`, `getBorrowedBooks()`, `setUsername()`, `setForename()`, `setSurname()`, `setEmail()`, `setPhoneNumber()`, `setPassword()`, `borrowBook()`, `returnBook()`, `checkOutStatus()`, and `getInfo()`.

- **Borrow and Return Books**:  
  - Track which user has borrowed which book and manage the due dates for returns.  
  - Implement transactions via a `Transaction` class that captures book borrowing and returning details along with timestamps.
  - Methods include `getID()`, `getType()`, `getStatus()`, `getBook()`, `getUser()`, `getDatetime()`, `setStatus()`, `setDatetime()`, `execute()`, `cancel()`, and `getInfo()`.

- **Networking for Concurrent Remote Access**:  
  - Enable remote access to the library system through a server-client architecture.  
  - Implement server-side functionality using the `Server` class and client-side operations using sockets.
  - The `Server` class handles client connections, manages threads for concurrent client handling, and communicates with the `Application` class.

- **Data Persistence with JSON**:
  - Enable data persistence through the `PersistenceManager` class which saves and loads data to and from the `Database`.
  - If there is no data to load, `Database` is populated with sample data for testing.

- **Additional Functionalities**:  
  - **Pagination**: Implement a method to navigate through long lists of books, users, and transactions efficiently using the `Menu` class.
  - **Approximate Search**: Enable search functionality with fuzzy matching using the `Database` class.

- **Documentation Generation**:  
  - Use Doxygen to generate comprehensive project documentation, detailing classes, methods, and relationships.

## Directory Structure

```
Library-Management-System/
├── CMakeLists.txt          # CMake build configuration file
├── doc/                    # Documentation directory
│   └── Doxyfile            # Doxygen configuration file
├── include/                # Header files
│   ├── Application.hpp     # Application class header
│   ├── Book.hpp            # Book class header
│   ├── Database.hpp        # Database class header
│   ├── Library.hpp         # LibraryManager class header
│   ├── Menu.hpp            # Menu class header
│   ├── Multithreading.hpp  # Multithreading utilities header
│   ├── Networking.hpp      # Networking utilities header
│   ├── Persistence.hpp     # Persistence utilities header
│   ├── Transaction.hpp     # Transaction class header
│   ├── User.hpp            # User class header
│   └── Utils.hpp           # Utility functions header
├── lib/                    # Source files
│   ├── Application.cpp     # Application class implementation
│   ├── Book.cpp            # Book class implementation
│   ├── Database.cpp        # Database class implementation
│   ├── Library.cpp         # LibraryManager class implementation
│   ├── main.cpp            # Main program entry point
│   ├── Menu.cpp            # Menu class implementation
│   ├── Multithreading.cpp  # Multithreading utilities implementation
│   ├── Networking.cpp      # Networking utilities implementation
│   ├── Persistence.cpp     # Persistence utilities implementation
│   ├── Transaction.cpp     # Transaction class implementation
│   ├── User.cpp            # User class implementation
│   └── Utils.cpp           # Utility functions implementation
└── tests/                  # Test files
    ├── ApplicationTest.cpp # Application class tests
    ├── BookTest.cpp        # Book class tests
    ├── DatabaseTest.cpp    # Database class tests
    ├── LibraryTest.cpp     # LibraryManager class tests
    ├── MenuTest.cpp        # Menu class tests
    ├── MultithreadingTest.cpp # Multithreading utilities tests
    ├── NetworkingTest.cpp  # Networking utilities tests
    ├── PersistenceTest.cpp # Persistence utilities tests
    ├── TransactionTest.cpp # Transaction class tests
    └── UserTest.cpp        # User class tests
```

## Installation Instructions

To install and set up the project locally, follow these steps:

1. **Run the build script**:
    ```bash
    ./make.sh
    ```

2. **Run the server**:
    ```bash
    ./build/LibraryManagementSystem
    ```

## Testing

This project uses CMake for the build and test process. The `make.sh` script simplifies the build process. To test the project, use one of the following commands:

1. **Build the project and run tests**:
    ```bash
    ./make.sh -t
    ```

2. **Build and run a specific test**:
    ```bash
    ./make.sh -t TestName
    ```

Replace `TestName` with the actual name of the test you want to run.

## Usage

To use the library management system, follow these steps:

1. **Connect a client to the server**:
    ```bash
    telnet localhost 8080
    ```
    **OR**
    ```bash
    nc localhost 8080
    ```

2. **Interact with the system**:
    - **Login**: Enter your username and password.
    - **Register**: Create a new user account.
    - **Borrow Book**: Borrow a book from the library.
    - **Return Book**: Return a borrowed book to the library.
    - **Search Items**: Search for books, users, or transactions.
    - **View My Profile**: View your user profile.
    - **Update My Profile**: Update your user profile.
    - **Add New Book**: Add a new book to the library (admin only).
    - **Add New User**: Add a new user to the system (admin only).