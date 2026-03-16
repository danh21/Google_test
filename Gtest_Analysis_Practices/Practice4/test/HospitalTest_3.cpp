#include <gtest/gtest.h>
#include "Hospital.hpp"
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>

/*
- Test Isolation (Flaky Tests): 
Lỗi rò rỉ trạng thái giữa các test case, khiến kết quả test phụ thuộc vào thứ tự chạy.
- Assertion Misuse (False Positives): 
Dùng sai macro của GTest (ví dụ: so sánh con trỏ thay vì giá trị).
- Exception Silencing (False Negatives): 
Catch exception nhưng quên đánh dấu test fail.
- Test Resource Leaks: 
uên giải phóng tài nguyên (bộ nhớ, file) do chính test case cấp phát.
- Vacuous Truth (Unreachable Assertions): 
Vòng lặp kiểm tra không bao giờ được thực thi, test luôn pass.
- Threading Pitfalls in GTest: 
Dùng ASSERT_* sai cách trong multi-threading hoặc helper function.
- Test Logic Miscalculation: 
Sai số do kiểu dữ liệu trong chính công thức tính "Kết quả mong đợi" (Expected value) của test case.
*/

// ============================================================================
// TEST FIXTURE SETUP
// ============================================================================
class HospitalTestFixture_2 : public ::testing::Test {
protected:
    std::unique_ptr<Hospital> hospital;
    int default_in_id;
    int default_out_id;

    void SetUp() override {
        hospital = std::make_unique<Hospital>("Grand Line Hospital");
        default_in_id = hospital->admitInPatient("Luffy", 19, "VIP1", 500.0, 2);
        default_out_id = hospital->registerOutPatient("Zoro", 21, 150.0);
    }

    void TearDown() override {
        hospital.reset();
    }
};

// ============================================================================
// 1. TEST ISOLATION / FLAKY TESTS (R� r? tr?ng th�i to�n c?c)
// ============================================================================
static double global_discount_rate = 1.0; 

TEST_F(HospitalTestFixture_2, Isolation_Easy) {

    global_discount_rate = 0.5; 
    EXPECT_EQ(global_discount_rate, 0.5);
}

TEST_F(HospitalTestFixture_2, Isolation_Medium) {
    auto p = hospital->findPatient(default_in_id);
    std::string report_file = "test_report.txt"; 

    std::ofstream out(report_file, std::ios::app);
    out << p->getSummary() << "\n";
    out.close();

    std::ifstream in(report_file);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    bool has_data = content.length() > 10;
    EXPECT_TRUE(has_data);
}

void processPatientData(std::shared_ptr<Patient> p, std::vector<std::string>& output) {
    static std::vector<int> processed_ids; 

    if (std::find(processed_ids.begin(), processed_ids.end(), p->getID()) == processed_ids.end()) {
        output.push_back(p->getName() + " Processed");
        processed_ids.push_back(p->getID());
    }
}

TEST_F(HospitalTestFixture_2, Isolation_Hard) {
    auto p = hospital->findPatient(default_in_id);
    std::vector<std::string> results;

    for (int i = 0; i < 5; ++i) {
        if (p->isActive() && p->getAge() > 0) {
            processPatientData(p, results);
        }
    }

    EXPECT_EQ(results.size(), 1); 
}

// ============================================================================
// 2. ASSERTION MISUSE (False Positives - So s�nh sai b?n ch?t)
// ============================================================================
TEST_F(HospitalTestFixture_2, AssertMisuse_Easy) {
    const char* expected_type = "InPatient";
    std::string actual_type = hospital->findPatient(default_in_id)->getType();

    EXPECT_NE(actual_type.c_str(), expected_type); 
}

TEST_F(HospitalTestFixture_2, AssertMisuse_Medium) {

    auto patient_by_id = hospital->findPatient(default_in_id);
    auto patient_by_name = hospital->getPatientsByName("Luffy").front();

    patient_by_id->addPrescription("Meat", "1 ton", 1, "Sanji");

    EXPECT_EQ(patient_by_id, patient_by_name); 
}

TEST_F(HospitalTestFixture_2, AssertMisuse_Hard) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));

    p->addProcedure("Scan", 100.0);
    p->addProcedure("Surgery", 500.0);
    p->extendStay(5);

    bool is_valid_bill = (p->getBill() > 0.0);
    bool is_valid_days = (p->getDaysAdmitted() == 7);
    bool has_procedures = (p->getProcedureCount() == 2);

    EXPECT_TRUE(is_valid_bill && is_valid_days && has_procedures);
}

// ============================================================================
// 3. EXCEPTION SILENCING
// ============================================================================
TEST_F(HospitalTestFixture_2, ExceptionSilencing_Easy) {
    try {

        auto p = hospital->findPatient(-1); 
        p->getName(); 
    } catch (...) {

    }
}

TEST_F(HospitalTestFixture_2, ExceptionSilencing_Medium) {
    auto p = hospital->findPatient(default_out_id);

    std::string valid_diagnosis = "Flu";
    double valid_fee = 50.0;

    auto executeAction = [&]() {
        auto op = std::dynamic_pointer_cast<OutPatient>(p);

        std::shared_ptr<OutPatient> null_op = nullptr; 
        null_op->addVisit(valid_diagnosis, valid_fee); 
    };

    EXPECT_ANY_THROW(executeAction());
}

int getPatientAgeSafe(Hospital* h, int id) {
    try {
        return h->findPatient(id)->getAge();
    } catch (const std::exception& e) {

        return 0; 
    }
}

TEST_F(HospitalTestFixture_2, ExceptionSilencing_Hard) {

    int age = getPatientAgeSafe(hospital.get(), 999);

    int current_year = 2026;
    int estimated_birth_year = current_year - age;

    EXPECT_EQ(estimated_birth_year, 2026);
}

// ============================================================================
// 4. TEST RESOURCE LEAKS
// ============================================================================
TEST_F(HospitalTestFixture_2, ResourceLeak_Easy) {

    int* expected_bill = new int(150); 

    auto p = hospital->findPatient(default_out_id);
    EXPECT_EQ(p->getBill(), *expected_bill);
}

TEST_F(HospitalTestFixture_2, ResourceLeak_Medium) {
    std::string dump_path = "patient_dump.tmp";

    std::ofstream out(dump_path);
    out << hospital->findPatient(default_in_id)->getSummary();
    out.close();

    FILE* file = fopen(dump_path.c_str(), "r");
    ASSERT_NE(file, nullptr);

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) != nullptr) {
        std::string line(buffer);
        EXPECT_TRUE(line.find("ID") != std::string::npos);
    }

}

struct TestObserver {
    std::shared_ptr<Patient> observed_patient;
    std::shared_ptr<TestObserver> self_ref; 
};

TEST_F(HospitalTestFixture_2, ResourceLeak_Hard) {
    auto p = hospital->findPatient(default_in_id);

    auto observer = std::make_shared<TestObserver>();
    observer->observed_patient = p;

    observer->self_ref = observer; 

    EXPECT_EQ(observer->observed_patient->getAge(), 19);
}

// ============================================================================
// 5. VACUOUS TRUTH
// ============================================================================
TEST_F(HospitalTestFixture_2, VacuousTruth_Easy) {
    auto patients = hospital->getAllPatients();

    for (size_t i = 0; i < 0; ++i) { 
        EXPECT_GT(patients[i]->getAge(), 0);
    }
}

TEST_F(HospitalTestFixture_2, VacuousTruth_Medium) {
    auto all_patients = hospital->getAllPatients();
    std::vector<std::shared_ptr<Patient>> seniors;

    for (auto p : all_patients) {
        if (p->getAge() >= 60) seniors.push_back(p);
    }

    double total_senior_bill = 0;
    for (auto s : seniors) {
        total_senior_bill += s->getBill();

        EXPECT_LE(s->getBill(), 100.0); 
    }

}

TEST_F(HospitalTestFixture_2, VacuousTruth_Hard) {
    auto p = hospital->findPatient(default_in_id);

    auto validatePatientRecord = [&](std::shared_ptr<Patient> patient) {
        if (!patient->isActive()) return; 

        if (patient->getAge() != 19) {

            return; 
        }

        EXPECT_EQ(patient->getName(), "Luffy");
    };

    hospital->admitInPatient("Sanji", 20, "V3", 100);
    validatePatientRecord(hospital->findPatient(3));
}

// ============================================================================
// 6. THREADING PITFALLS IN GTEST
// ============================================================================
TEST_F(HospitalTestFixture_2, ThreadPitfall_Easy) {
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        EXPECT_EQ(hospital->getTotalPatientCount(), 999); 
    });
    t.detach(); 
}

void checkHospitalName(Hospital* h) {

    ASSERT_EQ(h->getHospitalName(), "Wrong Name"); 

    h->registerOutPatient("Test", 20, 10); 
}

TEST_F(HospitalTestFixture_2, ThreadPitfall_Medium) {
    checkHospitalName(hospital.get());

    EXPECT_EQ(hospital->getTotalPatientCount(), 2); 
}

TEST_F(HospitalTestFixture_2, ThreadPitfall_Hard) {
    std::vector<std::thread> workers;

    for (int i = 0; i < 10; ++i) {
        workers.emplace_back([&, i]() {
            auto new_id = hospital->admitInPatient("Clone", 20, "1", 10.0);

            EXPECT_GT(new_id, 0); 
        });
    }

    for (auto& w : workers) { w.join(); }
}

// ============================================================================
// 7. TEST LOGIC MISCALCULATION
// ============================================================================
TEST_F(HospitalTestFixture_2, LogicCalc_Easy) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));

    double expected_discount_bill = (1 / 2) * (500.0 * 2); 

    EXPECT_EQ(p->getBill() * 0.5, expected_discount_bill);
}

TEST_F(HospitalTestFixture_2, LogicCalc_Medium) {
    auto p = hospital->findPatient(default_out_id);
    int current_year = 2026;

    std::string mock_dob_from_db = "2005-12-31"; 
    int birth_year = std::stoi(mock_dob_from_db.substr(0, 4));

    int age_in_months = (current_year - birth_year) * 12;
    int expected_age = age_in_months / 12;

    expected_age += 1; 

    EXPECT_EQ(p->getAge(), expected_age);
}

TEST_F(HospitalTestFixture_2, LogicCalc_Hard) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));

    int years = 50;
    int days_per_year = 365;
    int daily_rate = 500;

    p->extendStay(years * days_per_year);

    double expected_total_bill = (years * days_per_year * daily_rate) + (2 * 500); 

    EXPECT_DOUBLE_EQ(p->getBill(), expected_total_bill);
}