{#include <iostream>
#include <sstream>
#include <iomanip>//Has a function to set some precision for floating point numbers.
#include <string>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

//Create functions.
// Function to check if a given year is a leap year or not
bool isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Function to validate if a given date is valid
bool isValidDate(int day, int month, int year)
{
    // Check for basic conditions to validate the date
    if (year < 0 || month < 1 || month > 12 || day < 1)
    {
        return true;
    }

    // Array representing the number of days in each month (January to December)
    static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // Set the maximum days in the given month
    int maxDays = daysInMonth[month - 1];

    // If the given month is February and the year is a leap year, increment maxDays by 1
    if (month == 2 && isLeapYear(year))
    {
        maxDays++;
    }

    // Check if the day value is within the valid range for the given month
    return day <= maxDays;
}

sql::Connection* establishConnection()
{
    sql::mysql::MySQL_Driver* driver;
    sql::Connection* con = nullptr;

    std::string username = "your username";
    std::string password = "your password";
    std::string host = "127.0.0.1:3306";

    try
    {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(host, username, password);

        sql::Statement* stmt = con->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SHOW DATABASES LIKE 'simple_bank_db'");

        if (res->rowsCount() == 0)
        {
            stmt->execute("CREATE DATABASE simple_bank_db;");
            stmt->execute("USE simple_bank_db;");
            stmt->execute("CREATE TABLE user_info (user_id INT PRIMARY KEY AUTO_INCREMENT, full_name TEXT(255), id_no INT UNIQUE, phone_number VARCHAR(13) UNIQUE, date_of_birth DATE, email_address VARCHAR(70) UNIQUE, Nationality TEXT(20), City TEXT(50), County TEXT(50), password VARCHAR(20));");
            stmt->execute("ALTER TABLE user_info AUTO_INCREMENT = 32000001;");
            stmt->execute("USE simple_bank_db;");
            stmt->execute("CREATE TABLE checking_account(user_id INT UNIQUE NOT NULL REFERENCES user_info(user_id), is_active BOOLEAN, balance DECIMAL(9, 2));");
            stmt->execute("USE simple_bank_db;");
            stmt->execute("CREATE TABLE savings_account(user_id INT UNIQUE NOT NULL REFERENCES user_info(user_id), is_active BOOLEAN, withdrawals INT, balance DECIMAL(9, 2));");

        }

        delete res;
        delete stmt;

        con->setSchema("simple_bank_db");
    }
    catch (const sql::SQLException& e)
    {
        std::cout << "SQL Connection Error: " << e.what() << std::endl;
        if (con)
        {
            delete con;
            con = nullptr;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception occurred: " << e.what() << std::endl;
        if (con)
        {
            delete con;
            con = nullptr;
        }
    }
    catch (...)
    {
        std::cout << "Unknown exception occurred." << std::endl;
        if (con)
        {
            delete con;
            con = nullptr;
        }
    }

    return con;
}

void insertData(sql::Connection* con, const std::string& full_name, int id_no, const std::string& phone_number, const std::string& date_of_birth, const std::string& email_address, const std::string& Nationality, const std::string& City, const std::string& County, const std::string& password)
{
    try
    {
        sql::PreparedStatement* pstmt;
        pstmt = con->prepareStatement("INSERT INTO user_info(full_name, id_no, phone_number, "
            "date_of_birth, email_address, Nationality, City, County, password) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        pstmt->setString(1, full_name);
        pstmt->setInt(2, id_no);
        pstmt->setString(3, phone_number);
        pstmt->setString(4, date_of_birth);
        pstmt->setString(5, email_address);
        pstmt->setString(6, Nationality);
        pstmt->setString(7, City);
        pstmt->setString(8, County);
        pstmt->setString(9, password);
        pstmt->execute();
        delete pstmt;
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to insert data into the database: " + std::string(e.what()));
    }
}

void closeConnection(sql::Connection* con) {
    if (con) {
        delete con;
    }
}

void showBalance(double checking_balance)
{
    //To display the balance up to 2 decimal places, use (std::setprecision(2)), then add (std::fixed).
    std::cout << "Your Balance is: Sh." << std::setprecision(2) << std::fixed << checking_balance << "\n";
}

void showSavingsBalance(double savings_balance)
{
    //To display the balance up to 2 decimal places, use (std::setprecision(2)), then add (std::fixed).
    std::cout << "Your Balance is: Sh." << std::setprecision(2) << std::fixed << savings_balance << "\n";
}

double deposit()
{
    // Create a local variable called amount set to 0.
    double amount = 0;

    //Ask the user how much they need to deposit.
    std::cout << "How much do you want to deposit?";
    std::cin >> amount;

    // Prevent the user from entering a negative amount
    if (amount > 0)
    {
        return amount;
    }
    else
    {
        std::cout << "Invalid input.\n";
        return 0;
    }

}

double withdrawSavings(double savings_balance, int withdrawals, std::string user_name)
{

    sql::Connection* con = establishConnection(); // Create and initialize the connection

    if (con == nullptr)
    {
        std::cout << "Failed to establish database connection." << std::endl;
        return 1; // Exit the program due to connection failure
    }

    double amount = 0;

    std::cout << "How much do you want to withdraw? ";
    std::cin >> amount;

    if (amount > 0 && amount <= savings_balance)
    {
        // Check if the number of withdrawals is less than 4
        if (withdrawals < 4)
        {
            // Increment the number of withdrawals
            withdrawals++;

            try
            {
                // First, retrieve the user_id based on the provided email_address from the user_info table
                sql::PreparedStatement* pstmt;
                pstmt = con->prepareStatement("SELECT user_id FROM user_info WHERE email_address = ?");
                pstmt->setString(1, user_name);
                sql::ResultSet* res = pstmt->executeQuery();

                int user_id2 = -1; // Default value if user_id is not found

                if (res->next()) {
                    user_id2 = res->getInt("user_id");
                }
                else {
                    throw std::runtime_error("User not found for the provided email address.");
                }

                delete res;
                delete pstmt;

                // Update the number of withdrawals from the savings_accounts table based on the user_id.
                pstmt = con->prepareStatement("UPDATE savings_account SET withdrawals = ? WHERE user_id = ?");
                pstmt->setInt(1, withdrawals);
                pstmt->setInt(2, user_id2);
                res = pstmt->executeQuery(); // Use a different pointer for the result set.

                delete res;
                delete pstmt;
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error("Failed to update savings account table: " + std::string(e.what()));
            }

            return amount;
        }
        else
        {
            std::cout << "You have reached the maximum number of withdrawals (4) for this month.\n";
            return 0;
        }
    }
    else if (amount > savings_balance)
    {
        std::cout << "Insufficient funds.\n";
        return 0;
    }
    else
    {
        std::cout << "Invalid amount.\n";
        return 0;
    }

    delete con;

}

double withdraw(double checking_balance)
{
    double amount = 0;

    std::cout << "How much do you want to withdraw? ";
    std::cin >> amount;

    if (amount > 0 && amount <= checking_balance) // Using the local 'balance' variable
    {
        return amount;
    }
    else if (amount > checking_balance)
    {
        std::cout << "Insufficient funds.\n";
        return 0;
    }
    else
    {
        std::cout << "Invalid amount.\n";
        return 0;
    }
}

void createCheckingAcc(sql::Connection* con, const std::string& email_address, const double& balance)
{
    bool is_active = true; // Set is_active to true for a newly created account

    try
    {
        // First, retrieve the user_id based on the provided email_address from the user_info table
        sql::PreparedStatement* pstmt;
        pstmt = con->prepareStatement("SELECT user_id FROM user_info WHERE email_address = ?");
        pstmt->setString(1, email_address);
        sql::ResultSet* res = pstmt->executeQuery();

        int user_id = -1; // Default value if user_id is not found

        if (res->next()) {
            user_id = res->getInt("user_id");
        }
        else {
            throw std::runtime_error("User not found for the provided email address.");
        }

        delete res;
        delete pstmt;

        // Now, insert the checking account data into the checking_accounts table
        pstmt = con->prepareStatement("INSERT INTO checking_account(user_id, is_active, balance) "
            "VALUES (?, ?, ?)");
        pstmt->setInt(1, user_id);
        pstmt->setBoolean(2, is_active);
        pstmt->setDouble(3, balance);
        pstmt->execute();
        delete pstmt;

        std::cout << "You have successfully created a checking account.\n";

    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to create checking account: " + std::string(e.what()));
    }
}

void createSavingsAcc(sql::Connection* con, const std::string& email_address, const double balance)
{
    bool is_active = true; // Set is_active to true for a newly created account
    int withdrawals = 0;

    try
    {
        // First, retrieve the user_id based on the provided email_address from the user_info table
        sql::PreparedStatement* pstmt;
        pstmt = con->prepareStatement("SELECT user_id FROM user_info WHERE email_address = ?");
        pstmt->setString(1, email_address);
        sql::ResultSet* res = pstmt->executeQuery();

        int user_id = -1; // Default value if user_id is not found

        if (res->next()) {
            user_id = res->getInt("user_id");
        }
        else {
            throw std::runtime_error("User not found for the provided email address.");
        }

        delete res;
        delete pstmt;

        // Now, insert the checking account data into the checking_accounts table
        pstmt = con->prepareStatement("INSERT INTO savings_account(user_id, is_active, withdrawals, balance) "
            "VALUES (?, ?, ?, ?)");
        pstmt->setInt(1, user_id);
        pstmt->setBoolean(2, is_active);
        pstmt->setInt(3, withdrawals);
        pstmt->setDouble(4, balance);
        pstmt->execute();
        delete pstmt;

        std::cout << "You have successfully created a savings account.\n";

    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to create savings account: " + std::string(e.what()));
    }
}

double getCheckingAccountBalance(sql::Connection* con, std::string user_name)
{
    double balance_in_db{};

    try
    {
        // First, retrieve the user_id based on the provided email_address from the user_info table
        sql::PreparedStatement* pstmt;
        pstmt = con->prepareStatement("SELECT user_id FROM user_info WHERE email_address = ?");
        pstmt->setString(1, user_name);
        sql::ResultSet* res = pstmt->executeQuery();

        int user_id = -1; // Default value if user_id is not found

        if (res->next()) {
            user_id = res->getInt("user_id");
        }
        else {
            throw std::runtime_error("User not found for the provided email address.");
        }

        delete res;
        delete pstmt;

        // Retrieve the balance from the checking_accounts table based on the user_id.
        pstmt = con->prepareStatement("SELECT balance FROM checking_account WHERE user_id = ?");
        pstmt->setInt(1, user_id);
        res = pstmt->executeQuery(); // Use a different pointer for the result set

        if (res->next()) {
            balance_in_db = res->getDouble("balance");
        }

        delete res;
        delete pstmt;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to retrieve checking account balance: " + std::string(e.what()));
    }

    return balance_in_db;
}

void checkingAccount(sql::Connection* con, std::string user_name)
{
    try
    {
        // Retrieve the checking account balance from the database using the user's user_id
        double checking_balance = getCheckingAccountBalance(con, user_name);

        // Declare a local variable 'choice' to store the user's menu choice.
        int choice = 0;

        // Start a do-while loop that will continue until the user chooses to exit (chooses option 4).
        do
        {
            // Display the Checking Account Menu options to the user.
            std::cout << "Checking Account Menu:\n";
            std::cout << "1. Show Balance\n2. Deposit Money\n3. Withdraw Money\n4. Exit\nWhat do you want to do: ";

            // Read the user's choice and store it in the 'choice' variable.
            std::cin >> choice;

            // Check if the user input has failed (e.g., non-numeric input) and handle it.
            if (std::cin.fail())
            {
                // Clear the error state of the input.
                std::cin.clear();

                // Ignore any remaining characters in the input buffer until a newline is encountered.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                // Restart the loop to get a valid input from the user.
                continue;
            }

            // Ignore any remaining characters in the input buffer until a newline is encountered.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // Process the user's choice using a switch statement.
            switch (choice)
            {
                // Case 1: Show Balance
            case 1:
                // Call the 'showBalance()' function to display the checking account balance.
                showBalance(checking_balance);
                break;

                // Case 2: Deposit Money
            case 2:
                // Call the 'deposit()' function to get the amount to be deposited and add it to the balance.
                checking_balance += deposit();
                // Show the updated balance after the deposit operation.
                showBalance(checking_balance);
                break;

                // Case 3: Withdraw Money
            case 3:
                // Call the 'withdraw()' function to get the amount to be withdrawn and subtract it from the balance.
                checking_balance -= withdraw(checking_balance);
                // Show the updated balance after the withdrawal operation.
                showBalance(checking_balance);
                break;

                // Case 4: Exit
            case 4:
                // Display a message indicating that the user is exiting the checking account menu.
                std::cout << "Exiting Checking Account.\n";
                break;

                // Default case: Invalid input
            default:
                // Display a message indicating that the user input is not a valid option.
                std::cout << "That is not a valid input.\n";
                break;
            }

            // Continue the loop until the user chooses to exit (chooses option 4).
        } while (choice != 4);
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception occurred: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown exception occurred." << std::endl;
    }
}

double getSavingsAccountBalance(sql::Connection* con, std::string user_name)
{
    double balance_in_savingsDB{};

    try
    {
        // First, retrieve the user_id based on the provided email_address from the user_info table
        sql::PreparedStatement* pstmt;
        pstmt = con->prepareStatement("SELECT user_id FROM user_info WHERE email_address = ?");
        pstmt->setString(1, user_name);
        sql::ResultSet* res = pstmt->executeQuery();

        int user_id = -1; // Default value if user_id is not found

        if (res->next()) {
            user_id = res->getInt("user_id");
        }
        else {
            throw std::runtime_error("User not found for the provided email address.");
        }

        delete res;
        delete pstmt;

        // Retrieve the balance from the checking_accounts table based on the user_id.
        pstmt = con->prepareStatement("SELECT balance FROM savings_account WHERE user_id = ?");
        pstmt->setInt(1, user_id);
        res = pstmt->executeQuery(); // Use a different pointer for the result set

        if (res->next()) {
            balance_in_savingsDB = res->getDouble("balance");
        }

        delete res;
        delete pstmt;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to retrieve savings account balance: " + std::string(e.what()));
    }

    return balance_in_savingsDB;
}

int WithdrawalsCount(sql::Connection* con, std::string user_name)
{
    int withdrawals_in_db{};

    try
    {
        // First, retrieve the user_id based on the provided email_address from the user_info table
        sql::PreparedStatement* pstmt;
        pstmt = con->prepareStatement("SELECT user_id FROM user_info WHERE email_address = ?");
        pstmt->setString(1, user_name);
        sql::ResultSet* res = pstmt->executeQuery();

        int user_id = -1; // Default value if user_id is not found

        if (res->next()) {
            user_id = res->getInt("user_id");
        }
        else {
            throw std::runtime_error("User not found for the provided email address.");
        }

        delete res;
        delete pstmt;

        // Retrieve the balance from the checking_accounts table based on the user_id.
        pstmt = con->prepareStatement("SELECT withdrawals FROM savings_account WHERE user_id = ?");
        pstmt->setInt(1, user_id);
        res = pstmt->executeQuery(); // Use a different pointer for the result set

        if (res->next()) {
            withdrawals_in_db = res->getInt("withdrawals");
        }

        delete res;
        delete pstmt;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to retrieve number of withdrawals: " + std::string(e.what()));
    }

    return withdrawals_in_db;
}

void savingsAccount(sql::Connection* con, std::string user_name)
{
    try
    {
        double savings_balance = getSavingsAccountBalance(con, user_name);
        int withdrawals = WithdrawalsCount(con, user_name);
        int choice = 0;

        do
        {
            // Display the Checking Account Menu options to the user.
            std::cout << "Savings Account Menu:\n";
            std::cout << "1. Show Balance\n2. Deposit Money\n3. Withdraw Money\n4. Exit\nWhat do you want to do: ";
            std::cin >> choice;

            // Check if the user input has failed (e.g., non-numeric input) and handle it.
            if (std::cin.fail())
            {
                // Clear the error state of the input.
                std::cin.clear();

                // Ignore any remaining characters in the input buffer until a newline is encountered.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                // Restart the loop to get a valid input from the user.
                continue;
            }

            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // Process the user's choice using a switch statement.
            switch (choice)
            {
                // Case 1: Show Balance
            case 1:
                // Call the 'showBalance()' function to display the checking account balance.
                showSavingsBalance(savings_balance);
                break;

                // Case 2: Deposit Money
            case 2:
                // Call the 'deposit()' function to get the amount to be deposited and add it to the balance.
                savings_balance += deposit();
                // Show the updated balance after the deposit operation.
                showSavingsBalance(savings_balance);
                break;

                // Case 3: Withdraw Money
            case 3:
                if (withdrawals < 4) // Check if the number of withdrawals is less than 4
                {
                    savings_balance -= withdrawSavings(savings_balance, withdrawals, user_name);

                    showSavingsBalance(savings_balance);
                }
                else
                {
                    std::cout << "You have reached the maximum number of withdrawals (4) for this month.\n";
                }
                break;

                // Case 4: Exit
            case 4:
                // Display a message indicating that the user is exiting the checking account menu.
                std::cout << "Exiting Checking Account.\n";
                break;

                // Default case: Invalid input
            default:
                // Display a message indicating that the user input is not a valid option.
                std::cout << "That is not a valid input.\n";
                break;
            }
        } while (choice != 4);
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception occurred: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown exception occurred." << std::endl;
    }
}

void banking(std::string user_name, int user_id)
{
    sql::Connection* con = establishConnection(); // Create and initialize the connection

    if (con == nullptr)
    {
        std::cout << "Failed to establish database connection." << std::endl;
    }

    int choice = 0;

    do
    {
        std::cout << "1. Checking Account\n2. Savings Account\n3. Exit\nWhat do you want to do: ";
        std::cin >> choice;

        if (std::cin.fail())
        {
            // Clear the error state of the input.
            std::cin.clear();

            // Ignore any remaining characters in the input buffer until a newline is encountered.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // Restart the loop to get a valid input from the user.
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Process the user's choice using a switch statement.
        switch (choice)
        {
            // Case 1: Open the checking account.
        case 1:
            // Call the 'checkingAccount()' function to open the checking account.
            checkingAccount(con, user_name);
            break;

            // Case 2: Open the savings account.
        case 2:
            // Call the 'savingsAccount()' function to open the savings account.
            savingsAccount(con, user_name);
            break;

            // Case 3: Close the program.
        case 3:
            std::cout << "Thank you for visiting.";
            break;
        }

    } while (choice != 3);

    delete con;
}

// Function to collect user information.
void get_userInfo()
{
    // Variables to store user information
    int id_no;
    std::string first_name,
        middle_name,
        last_name,
        phone_number,
        day,
        month,
        year,
        date_of_birth,
        email_address,
        City, County,
        Nationality,
        full_name,
        password1,
        password2;

    // Prompt user for input and read values
    std::cout << "First Name: ";
    std::cin >> first_name;

    std::cout << "Middle Name: ";
    std::cin >> middle_name;

    std::cout << "Last Name: ";
    std::cin >> last_name;
    // Combine the first name, middle name, and last name to create the full name
    full_name = first_name + " " + middle_name + " " + last_name;

    std::cout << "ID Number: ";
    std::cin >> id_no;

    std::cout << "Phone Number: ";
    std::cin >> phone_number;

    std::cout << "Enter your date of Birth.\n";
    std::cout << "Day: ";
    std::cin >> day;

    std::cout << "Month: ";
    std::cin >> month;

    std::cout << "Year: ";
    std::cin >> year;

    // Convert day, month, and year to strings
    std::stringstream ss;
    ss << day;
    std::string dayStr = ss.str();
    ss.str(""); // Clear the stringstream
    ss << month;
    std::string monthStr = ss.str();
    ss.str("");
    ss << year;
    std::string yearStr = ss.str();

    // Concatenate the strings to form the date_of_birth
    date_of_birth = yearStr + "-" + monthStr + "-" + dayStr;

    std::cout << "Email Address: ";
    std::cin >> email_address;

    std::cout << "Nationality: ";
    std::cin >> Nationality;

    std::cout << "City: ";
    std::cin >> City;

    std::cout << "County: ";
    std::cin >> County;

    do {
        std::cout << "Enter Password: ";
        std::cin >> password1;

        std::cout << "Re-enter Password: ";
        std::cin >> password2;
        if (password2 != password1)
        {
            std::cout << "Passwords do not match.\n";
        }
    } while (password2 != password1);

    // Now that we have all the user information, let's store it in the database
    sql::Connection* con = establishConnection();
    if (con)
    {

        insertData(con, full_name, id_no, phone_number,
            date_of_birth, email_address, Nationality, City, County, password1);
        closeConnection(con);
        std::cout << "Connected to the database. Information stored." << std::endl;
    }
    else
    {
        std::cout << "Failed to connect to the database. Information not stored." << std::endl;
    }
}

void user_login(int user_id)
{
    std::string user_name, password;
    bool try_login = true; // Flag to control the loop
    int login_attempts = 0; // Counter for login attempts
    bool user_locked = false; // Flag to indicate if the user is locked out

    std::cout << "Please log in.\n";

    do {
        if (user_locked) {
            std::cout << "You have been locked out. Please contact support for assistance.\n";
            break; // Exit the loop if the user is locked out
        }

        std::cout << "Enter Email: ";
        std::cin >> user_name;

        std::cout << "Password: ";
        std::cin >> password;

        // Now, we need to check if the user-provided email and password match the records in the database
        // We'll use the same MySQL Connector/C++ library to perform a query to the database

        sql::Connection* con = establishConnection();
        if (con) {
            bool login_success = false;
            try {
                // Prepare a query to fetch the password and full name for the given email from the database
                sql::PreparedStatement* pstmt;
                pstmt = con->prepareStatement("SELECT password, full_name FROM user_info WHERE email_address = ?");
                pstmt->setString(1, user_name);
                sql::ResultSet* res = pstmt->executeQuery();

                // Check if there is any result returned from the query
                if (res->next()) {
                    std::string stored_password = res->getString("password");
                    std::string full_name = res->getString("full_name");

                    // Compare the stored password with the user-provided password
                    if (stored_password == password) {
                        std::cout << "Login successful. Welcome, " << full_name << "!\n";
                        login_success = true;
                    }
                    else {
                        std::cout << "Incorrect password. Login failed.\n";
                    }
                }
                else {
                    std::cout << "Email address not found. Login failed.\n";
                }

                delete res;
                delete pstmt;
            }
            catch (const sql::SQLException& e)
            {
                std::cout << "SQL Exception: " << e.what() << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cout << "Exception occurred: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cout << "Unknown exception occurred." << std::endl;
            }

            closeConnection(con);

            if (login_success) {
                // Call the banking function only if login is successful
                banking(user_name, user_id);
                try_login = false; // Set the flag to false to exit the outer loop
            }
            else {
                // Increment the login_attempts counter
                login_attempts++;
                if (login_attempts >= 3) {
                    std::cout << "Maximum login attempts reached. You have been locked out.\n";
                    user_locked = true; // Set the flag to true to indicate user lockout
                }
                else {
                    std::cout << "You have " << (3 - login_attempts) << " attempts left.\n";
                }
            }
        }
        else {
            std::cout << "Failed to connect to the database. Login failed." << std::endl;
        }

        // Prompt the user if they want to try logging in again (only if they are not locked out)
        if (try_login && !user_locked) {
            std::cout << "Do you want to try again? (Y/N): ";
            char try_again;
            std::cin >> try_again;
            if (try_again == 'N' || try_again == 'n') {
                try_login = false; // Set the flag to false to exit the outer loop
            }
        }
    } while (try_login);
}

int main(int user_id)
{
    sql::Connection* con = establishConnection(); // Create and initialize the connection

    if (con == nullptr)
    {
        std::cout << "Failed to establish database connection." << std::endl;
        return 1; // Exit the program due to connection failure
    }

    int user, choice;
    std::string email_address;
    double balance;

    do
    {
        std::cout << "WELCOME TO THE BANKING PROGRAM.\n\n";
        std::cout << "1. New User.\n2. Existing User.\n3. Exit\nNew or Existing user: ";
        std::cin >> user;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        try
        {
            switch (user)
            {
            case 1:
                get_userInfo();

                std::cout << "1. Create Checking Account\n2. Create Savings Account\n3. Exit\nWelcome! What do yo want to do? ";
                std::cin >> choice;
                if (std::cin.fail())
                {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    continue;
                }
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                try
                {
                    switch (choice)
                    {
                    case 1:
                        std::cout << "Enter your email address: ";
                        std::cin >> email_address;

                        do
                        {
                            std::cout << "You have to deposit at least Ksh.500 to activate the account.\n";
                            std::cout << "How much do you want to deposit? ";
                            std::cin >> balance;
                        } while (balance < 500);

                        createCheckingAcc(con, email_address, balance);
                        break;
                    case 2:
                        std::cout << "Enter your email address: ";
                        std::cin >> email_address;

                        do
                        {
                            std::cout << "You have to deposit at least Ksh.500 to activate the account.\n";
                            std::cout << "How much do you want to deposit? ";
                            std::cin >> balance;
                        } while (balance < 500);

                        createSavingsAcc(con, email_address, balance);
                        break;
                    case 3:
                        std::cout << "Thank you for visiting.";
                        break;
                    default:
                        break;
                    }


                }
                catch (const std::exception& e)
                {
                    std::cout << "Exception occurred: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cout << "Unknown exception occurred." << std::endl;
                }

                break;
            case 2:
                user_login(user_id);
                break;
            case 3:
                std::cout << "Thanks for visiting.\n";
                break;
            default:
                std::cout << "Invalid input.\n";
                break;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "Exception occurred: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cout << "Unknown exception occurred." << std::endl;
        }
    } while (user != 3);

    // Clean up the connection before exiting
    delete con;

    return 0;
}