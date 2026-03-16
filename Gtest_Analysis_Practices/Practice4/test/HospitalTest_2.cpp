#include <gtest/gtest.h>
#include "Hospital.hpp"
#include <thread>
#include <future>
#include <string_view>
#include <numeric>
#include <cstring>
#include <algorithm>

/*
- Segmentation Fault (Truy cập bộ nhớ trái phép):
Test case cố gắng đọc/ghi vào vùng bộ nhớ không được phép (con trỏ NULL, mảng vượt chỉ số), khiến chương trình test bị "sập" ngay lập tức.
- Use After Free (Sử dụng sau khi giải phóng):
Test case truy cập vào vùng bộ nhớ đã bị delete hoặc free, dẫn đến kết quả không xác định hoặc crash ngẫu nhiên.
- Data Corruption (Ghi đè dữ liệu):
Việc viết code test không cẩn thận làm thay đổi các biến toàn cục hoặc vùng nhớ dùng chung, gây sai lệch kết quả của các hàm logic khác.
- Boundary (Vi phạm biên):
Lỗi không kiểm tra các giá trị biên (Min/Max, Null, Empty), dẫn đến việc bỏ lỡ các kịch bản thực tế mà phần mềm có thể gặp phải.
- Memory Overrun (Tràn bộ nhớ):
Dữ liệu ghi vào mảng hoặc buffer vượt quá kích thước cấp phát, làm hỏng các dữ liệu liền kề trong bộ nhớ.
- Timeout (Quá thời gian thực thi):
Test case rơi vào vòng lặp vô tận hoặc đợi một sự kiện không bao giờ xảy ra, làm treo toàn bộ tiến trình CI/CD.
- Multi-declared (Khai báo trùng lặp):
Lỗi định nghĩa lại các hàm Mock hoặc biến toàn cục trong nhiều file test khác nhau, gây ra lỗi xung đột khi link (Linker Error).
*/

class HospitalTestFixture : public ::testing::Test {
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
// 1. SEGMENTATION FAULT (Dereferencing Null/Invalid Pointers)
// ============================================================================
TEST_F(HospitalTestFixture, SegFault_Easy) {
    auto p = hospital->findPatient(99); 

    EXPECT_EQ(p->getAge(), 19); 
}

TEST_F(HospitalTestFixture, SegFault_Medium) {
    std::vector<std::string> target_names = {"Sanji", "Nami", "Usopp", "Robin"};
    std::vector<std::shared_ptr<Patient>> search_results;

    for (const auto& name : target_names) {
        auto res = hospital->getPatientsByName(name);
        search_results.insert(search_results.end(), res.begin(), res.end());
    }

    int active_count = 0;
    for (auto& p : search_results) { 
        if (p && p->isActive()) active_count++; 
    }

    auto first_patient = search_results.front(); 
    EXPECT_EQ(first_patient->getName(), "Sanji");
}

TEST_F(HospitalTestFixture, SegFault_Hard) {
    auto base_ptr = hospital->findPatient(default_out_id); 
    bool is_eligible = base_ptr->getAge() >= 18 && base_ptr->isActive();
    double discount_applied = 0.0;

    if (is_eligible) {
        discount_applied = 50.0;
        base_ptr->setName("Zoro - Surgery Prep");
    }

    InPatient* fake_in_patient = static_cast<InPatient*>(base_ptr.get());
    fake_in_patient->addProcedure("Sword wound care", 300.0 - discount_applied);

    EXPECT_GT(fake_in_patient->getProcedureCost(), 0.0);
}

// ============================================================================
// 2. USE AFTER FREE (Dangling Pointers / References)
// ============================================================================
TEST_F(HospitalTestFixture, UAF_Easy) {
    const char* name_ptr = hospital->findPatient(default_in_id)->getName().c_str();

    EXPECT_STREQ(name_ptr, "Luffy");
}

TEST_F(HospitalTestFixture, UAF_Medium) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    p->addProcedure("Initial Assessment", 100.0);
    p->addProcedure("Blood Work", 50.0);

    auto getImportantProcedure = [&]() -> const std::string& {
        return p->getProcedures()[0].name;
    };

    const std::string& important_proc = getImportantProcedure();
    double total_extra_cost = 0.0;

    for(int i = 0; i < 50; ++i) {
        p->addProcedure("Routine Check " + std::to_string(i), 15.0);
        total_extra_cost += 15.0;
    }

    EXPECT_EQ(important_proc, "Initial Assessment"); 
    EXPECT_GT(total_extra_cost, 0);
}

TEST_F(HospitalTestFixture, UAF_Hard) {
    auto p = hospital->findPatient(default_in_id);
    std::vector<std::string_view> patient_tags;

    auto extractTags = [&](std::shared_ptr<Patient> patient) {

        std::string_view sv = patient->getSummary(); 
        if (sv.find("VIP1") != std::string_view::npos) {
            patient_tags.push_back(sv.substr(0, 10)); 
        }
    };

    extractTags(p);

    std::vector<int> random_data(1000);
    std::iota(random_data.begin(), random_data.end(), 1);
    int sum = std::accumulate(random_data.begin(), random_data.end(), 0);

    EXPECT_FALSE(patient_tags.empty());
    EXPECT_GT(sum, 0);
}

// ============================================================================
// 3. DATA CORRUPTION (Undefined Behavior / Race Conditions)
// ============================================================================
TEST_F(HospitalTestFixture, DataCorrupt_Easy) {
    static int local_patient_count = 0; 
    local_patient_count += hospital->getTotalPatientCount();

    EXPECT_EQ(local_patient_count, 2);
}

TEST_F(HospitalTestFixture, DataCorrupt_Medium) {
    auto p = hospital->findPatient(default_in_id);
    p->addPrescription("Aspirin", "1 pill", 7, "Dr. Chopper");
    p->addPrescription("Vitamin C", "2 pills", 14, "Dr. Chopper");

    Prescription* cache_buffer = static_cast<Prescription*>(malloc(2 * sizeof(Prescription)));
    const auto& original_prescriptions = p->getPrescriptions();

    for (size_t i = 0; i < original_prescriptions.size(); ++i) {
        memcpy(&cache_buffer[i], &original_prescriptions[i], sizeof(Prescription)); 
    }

    EXPECT_EQ(cache_buffer[0].medicine, "Aspirin");
    free(cache_buffer);
}

TEST_F(HospitalTestFixture, DataCorrupt_Hard) {
    int total_tasks = 2000;
    std::atomic<int> completed_tasks{0};

    auto workerThread = [&]() {
        for(int i = 0; i < total_tasks / 2; ++i) {

            std::string name = "Clone_" + std::to_string(i);
            hospital->registerOutPatient(name, 20 + (i % 30), 10.0 + (i % 5));
            completed_tasks++;
        }
    };

    auto future1 = std::async(std::launch::async, workerThread);
    auto future2 = std::async(std::launch::async, workerThread);

    future1.get(); 
    future2.get();

    EXPECT_EQ(completed_tasks.load(), 2000);
    EXPECT_EQ(hospital->getTotalPatientCount(), 2002); 
}

// ============================================================================
// 4. BOUNDARY (Off-by-one / Limits / Precision)
// ============================================================================
TEST_F(HospitalTestFixture, Boundary_Easy) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));

    EXPECT_NO_THROW(p->extendStay(0)); 
}

TEST_F(HospitalTestFixture, Boundary_Medium) {
    auto p = hospital->findPatient(default_in_id);
    std::string prefix = "PAT_";
    std::string full_id_str = prefix + std::to_string(p->getID()) + "_2026"; 

    char generated_code[10];
    int current_index = 0;

    for (char c : full_id_str) {
        if (current_index <= 10) { 
            generated_code[current_index] = c;
            current_index++;
        }
    }

    EXPECT_EQ(generated_code[0], 'P');
}

TEST_F(HospitalTestFixture, Boundary_Hard) {
    auto p = std::dynamic_pointer_cast<OutPatient>(hospital->findPatient(default_out_id));
    double daily_surcharge = 0.1;
    int visit_days = 30;

    for (int i = 1; i <= visit_days; ++i) {
        bool is_weekend = (i % 7 == 6) || (i % 7 == 0);
        double actual_fee = is_weekend ? (daily_surcharge * 1.5) : daily_surcharge;
        p->addVisit("Routine Day " + std::to_string(i), actual_fee);
    }

    double expected_bill = 150.0;
    for (int i = 1; i <= visit_days; ++i) {
        expected_bill += 150.0 + ((i % 7 == 6 || i % 7 == 0) ? 0.15 : 0.1);
    }

    EXPECT_EQ(p->getBill(), expected_bill); 
}

// ============================================================================
// 5. MEMORY OVERRUN (Buffer Overflow)
// ============================================================================
TEST_F(HospitalTestFixture, MemOverrun_Easy) {
    int active_ids[2];
    auto patients = hospital->getAllPatients();

    for(size_t i = 0; i <= patients.size(); ++i) { 
        if(i < 2) active_ids[i] = patients[i]->getID();
    }
    EXPECT_EQ(active_ids[0], default_in_id);
}

TEST_F(HospitalTestFixture, MemOverrun_Medium) {
    auto patients = hospital->getAllPatients();
    auto it = patients.begin();

    size_t total_count = patients.size();
    size_t target_offset = total_count + 1; 

    if (target_offset > 0) {
        std::advance(it, target_offset); 
    }

    std::string found_name = "";

    if (it != patients.end()) { 
        found_name = (*it)->getName();
    }

    EXPECT_NE(found_name, "Unknown"); 
}

TEST_F(HospitalTestFixture, MemOverrun_Hard) {
    auto all_patients = hospital->getAllPatients();
    std::vector<std::shared_ptr<Patient>> active_patients;

    active_patients.reserve(all_patients.size()); 

    auto is_active_pred = [](const std::shared_ptr<Patient>& p) {
        return p->isActive() && p->getAge() > 10;
    };

    std::copy_if(all_patients.begin(), all_patients.end(), 
                 active_patients.begin(), is_active_pred);

    EXPECT_GE(active_patients.size(), 1);
}

// ============================================================================
// 6. TIMEOUT (Infinite Loops)
// ============================================================================
TEST_F(HospitalTestFixture, Timeout_Easy) {
    int days = 0;
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));

    while (p->getDaysAdmitted() < 5) { days++; }
    EXPECT_EQ(days, 3);
}

TEST_F(HospitalTestFixture, Timeout_Medium) {
    int max_capacity = 100;

    for (size_t i = 0; i < hospital->getAllPatients().size(); ++i) {
        auto current_patient = hospital->getAllPatients()[i];

        if (current_patient->getAge() < 20 && hospital->getTotalPatientCount() < max_capacity) {
            hospital->registerOutPatient("Relative of " + current_patient->getName(), 40, 50.0);
        }
    }

    EXPECT_EQ(hospital->getTotalPatientCount(), 3);
}

TEST_F(HospitalTestFixture, Timeout_Hard) {
    auto inpatient = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    double target_revenue = 2000.0;

    auto performDailyBilling = [&]() {
        inpatient->addProcedure("Daily Care", 50.0);
        if (inpatient->getProcedureCost() > 300.0) {

            inpatient->addProcedure("Discount", -50.0); 
        }
    };

    while (inpatient->getBill() < target_revenue) {
        performDailyBilling();
    }

    EXPECT_GE(inpatient->getBill(), target_revenue);
}

// ============================================================================
// 7. MULTI DECLARED (Shadowing / Macro / GTest Structure Errors)
// ============================================================================
#define admitInPatient(name, age, room, rate) printf("Admitted")
TEST_F(HospitalTestFixture, MultiDecl_Easy) {

    int id = hospital->admitInPatient("Nami", 20, "V2", 200.0); 
    EXPECT_GT(id, 0);
}
#undef admitInPatient

TEST_F(HospitalTestFixture, MultiDecl_Medium) {
    EXPECT_TRUE(true);
}

TEST_F(HospitalTestFixture, MultiDecl_Hard) {
    int target_age = 0;
    std::string target_name = "";
    bool parse_success = false;

    auto extractData = [&]() {
        try {
            auto p = hospital->findPatient(default_in_id);
            if (!p) throw std::runtime_error("Patient not found");

            int target_age = p->getAge(); 
            std::string target_name = p->getName();

            if (target_age > 0) {
                parse_success = true;
            }
        } catch (const std::exception& e) {
            parse_success = false;
        }
    };

    extractData();

    EXPECT_TRUE(parse_success);
    EXPECT_EQ(target_age, 19); 
    EXPECT_EQ(target_name, "Luffy");
}