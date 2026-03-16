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

// ============================================================================
// TEST FIXTURE SETUP
// ============================================================================
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

// [Dễ] Giả định sai về ID trả về.
TEST_F(HospitalTestFixture, SegFault_Easy) {
    auto p = hospital->findPatient(99); 
    // Rootcause: p là nullptr do không tìm thấy ID 99.
    // Fix: Thêm ASSERT_NE(p, nullptr); trước khi dereference.
    EXPECT_EQ(p->getAge(), 19); 
}

// [Trung bình] Gọi hàm front() trên container rỗng bị che giấu bởi vòng lặp.
TEST_F(HospitalTestFixture, SegFault_Medium) {
    std::vector<std::string> target_names = {"Sanji", "Nami", "Usopp", "Robin"};
    std::vector<std::shared_ptr<Patient>> search_results;
    
    // Giả lập logic tìm kiếm hàng loạt bệnh nhân từ một danh sách tên
    for (const auto& name : target_names) {
        auto res = hospital->getPatientsByName(name);
        search_results.insert(search_results.end(), res.begin(), res.end());
    }
    
    int active_count = 0;
    for (auto& p : search_results) { 
        if (p && p->isActive()) active_count++; 
    }
    
    // Rootcause: Không có tên nào tồn tại trong SetUp, search_results rỗng. Gọi front() gây SegFault.
    // Fix: Thêm ASSERT_FALSE(search_results.empty()); trước khi lấy phần tử đầu tiên.
    auto first_patient = search_results.front(); 
    EXPECT_EQ(first_patient->getName(), "Sanji");
}

// [Khó] Ép kiểu tĩnh (Static Cast) sai bản chất đa hình kẹp giữa business logic.
TEST_F(HospitalTestFixture, SegFault_Hard) {
    auto base_ptr = hospital->findPatient(default_out_id); 
    bool is_eligible = base_ptr->getAge() >= 18 && base_ptr->isActive();
    double discount_applied = 0.0;
    
    // Trainee dễ bị phân tâm bởi đoạn logic tính toán discount này
    if (is_eligible) {
        discount_applied = 50.0;
        base_ptr->setName("Zoro - Surgery Prep");
    }
    
    // Rootcause: default_out_id là OutPatient, ép tĩnh sang InPatient sẽ sai cấu trúc class. 
    // Trình biên dịch cho qua, nhưng lúc chạy gọi addProcedure sẽ truy cập sai vùng nhớ gây Crash.
    // Fix: Dùng auto in_p = std::dynamic_pointer_cast<InPatient>(base_ptr); và check if(in_p).
    InPatient* fake_in_patient = static_cast<InPatient*>(base_ptr.get());
    fake_in_patient->addProcedure("Sword wound care", 300.0 - discount_applied);
    
    EXPECT_GT(fake_in_patient->getProcedureCost(), 0.0);
}

// ============================================================================
// 2. USE AFTER FREE (Dangling Pointers / References)
// ============================================================================

// [Dễ] Lỗi c_str() vòng đời ngắn ẩn trong macro.
TEST_F(HospitalTestFixture, UAF_Easy) {
    const char* name_ptr = hospital->findPatient(default_in_id)->getName().c_str();
    // Rootcause: string tạm bị hủy sau dòng lệnh trên, name_ptr trỏ vào rác.
    // Fix: Lưu std::string name = ...; rồi dùng name.c_str() ở hàm EXPECT.
    EXPECT_STREQ(name_ptr, "Luffy");
}

// [Trung bình] Dangling reference qua tham chiếu ẩn của vector reallocation.
TEST_F(HospitalTestFixture, UAF_Medium) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    p->addProcedure("Initial Assessment", 100.0);
    p->addProcedure("Blood Work", 50.0);
    
    auto getImportantProcedure = [&]() -> const std::string& {
        return p->getProcedures()[0].name;
    };
    
    const std::string& important_proc = getImportantProcedure();
    double total_extra_cost = 0.0;
    
    // Vòng lặp này chèn thêm rất nhiều procedure làm vector vượt capacity cũ
    for(int i = 0; i < 50; ++i) {
        p->addProcedure("Routine Check " + std::to_string(i), 15.0);
        total_extra_cost += 15.0;
    }
    
    // Rootcause: Khi vector cấp phát lại vùng nhớ, biến tham chiếu important_proc trỏ vào vùng nhớ cũ (đã free).
    // Fix: Sửa lambda trả về chuỗi by value (std::string) thay vì tham chiếu (const std::string&).
    EXPECT_EQ(important_proc, "Initial Assessment"); 
    EXPECT_GT(total_extra_cost, 0);
}

// [Khó] Cạm bẫy của C++17 std::string_view kẹp trong data processing.
TEST_F(HospitalTestFixture, UAF_Hard) {
    auto p = hospital->findPatient(default_in_id);
    std::vector<std::string_view> patient_tags;
    
    auto extractTags = [&](std::shared_ptr<Patient> patient) {
        // Hàm getSummary() trả về rvalue std::string
        std::string_view sv = patient->getSummary(); 
        if (sv.find("VIP1") != std::string_view::npos) {
            patient_tags.push_back(sv.substr(0, 10)); // Lấy 10 kí tự đầu làm tag
        }
    };
    
    extractTags(p);
    
    // Giả lập một số thao tác xử lý dữ liệu khác tốn thời gian
    std::vector<int> random_data(1000);
    std::iota(random_data.begin(), random_data.end(), 1);
    int sum = std::accumulate(random_data.begin(), random_data.end(), 0);
    
    // Rootcause: getSummary() tạo ra chuỗi tạm, sv (string_view) giữ con trỏ của chuỗi tạm đó. 
    // Rời khỏi extractTags, chuỗi tạm bị hủy, các string_view trong patient_tags thành dangling.
    // Fix: patient_tags phải chứa std::string thay vì std::string_view.
    EXPECT_FALSE(patient_tags.empty());
    EXPECT_GT(sum, 0);
}

// ============================================================================
// 3. DATA CORRUPTION (Undefined Behavior / Race Conditions)
// ============================================================================

// [Dễ] Biến static giữ state giữa các test case.
TEST_F(HospitalTestFixture, DataCorrupt_Easy) {
    static int local_patient_count = 0; 
    local_patient_count += hospital->getTotalPatientCount();
    // Rootcause: Biến static không reset khi chạy lại.
    // Fix: Bỏ từ khóa static.
    EXPECT_EQ(local_patient_count, 2);
}

// [Trung bình] Phá hỏng chuỗi (string) bằng memcpy khi mô phỏng object pool.
TEST_F(HospitalTestFixture, DataCorrupt_Medium) {
    auto p = hospital->findPatient(default_in_id);
    p->addPrescription("Aspirin", "1 pill", 7, "Dr. Chopper");
    p->addPrescription("Vitamin C", "2 pills", 14, "Dr. Chopper");
    
    // Giả lập cơ chế cache hoặc object pool bằng cấp phát mảng thô
    Prescription* cache_buffer = static_cast<Prescription*>(malloc(2 * sizeof(Prescription)));
    const auto& original_prescriptions = p->getPrescriptions();
    
    // Thao tác copy nhanh (fast copy) thường thấy ở code cũ (C-style)
    for (size_t i = 0; i < original_prescriptions.size(); ++i) {
        memcpy(&cache_buffer[i], &original_prescriptions[i], sizeof(Prescription)); 
    }
    
    // Rootcause: Prescription chứa std::string. Copy byte-to-byte (memcpy) làm hai đối tượng 
    // trỏ chung vùng nhớ nội bộ của chuỗi. Khi giải phóng sẽ gây lỗi Double Free / Memory Corruption.
    // Fix: Dùng toán tử gán (cache_buffer[i] = ...) và khởi tạo bằng new[] thay vì malloc.
    EXPECT_EQ(cache_buffer[0].medicine, "Aspirin");
    free(cache_buffer);
}

// [Khó] Đua dữ liệu (Race condition) trong luồng giả lập tải nặng.
TEST_F(HospitalTestFixture, DataCorrupt_Hard) {
    int total_tasks = 2000;
    std::atomic<int> completed_tasks{0};
    
    auto workerThread = [&]() {
        for(int i = 0; i < total_tasks / 2; ++i) {
            // Giả lập thao tác xử lý nghiệp vụ trước khi đăng ký
            std::string name = "Clone_" + std::to_string(i);
            hospital->registerOutPatient(name, 20 + (i % 30), 10.0 + (i % 5));
            completed_tasks++;
        }
    };
    
    auto future1 = std::async(std::launch::async, workerThread);
    auto future2 = std::async(std::launch::async, workerThread);
    
    future1.get(); 
    future2.get();
    
    // Rootcause: std::vector trong Hospital không thread-safe. Hai luồng async cùng đẩy 
    // dữ liệu vào patients vector sẽ gây đụng độ vùng nhớ (Data Race), làm hỏng size/capacity.
    // Fix: Cần thêm mutex lock vào trong hàm registerOutPatient của class Hospital.
    EXPECT_EQ(completed_tasks.load(), 2000);
    EXPECT_EQ(hospital->getTotalPatientCount(), 2002); 
}

// ============================================================================
// 4. BOUNDARY (Off-by-one / Limits / Precision)
// ============================================================================

// [Dễ] Bỏ qua biên của Exception.
TEST_F(HospitalTestFixture, Boundary_Easy) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    // Rootcause: extendStay(0) sẽ ném std::invalid_argument theo logic class InPatient.
    // Fix: EXPECT_THROW(p->extendStay(0), std::invalid_argument);
    EXPECT_NO_THROW(p->extendStay(0)); 
}

// [Trung bình] Off-by-one do kích thước bộ đệm cố định khi sinh mã bệnh nhân.
TEST_F(HospitalTestFixture, Boundary_Medium) {
    auto p = hospital->findPatient(default_in_id);
    std::string prefix = "PAT_";
    std::string full_id_str = prefix + std::to_string(p->getID()) + "_2026"; 
    
    char generated_code[10];
    int current_index = 0;
    
    // Logic điền tay từng kí tự vào mảng để tạo mã
    for (char c : full_id_str) {
        if (current_index <= 10) { 
            generated_code[current_index] = c;
            current_index++;
        }
    }
    
    // Rootcause: current_index <= 10 cho phép vòng lặp chạy khi index = 10, ghi vào generated_code[10]
    // làm tràn biên mảng (Out of bounds) trên stack.
    // Fix: Đổi điều kiện thành current_index < 10.
    EXPECT_EQ(generated_code[0], 'P');
}

// [Khó] Tích lũy sai số phẩy động ngầm định kết hợp logic giảm giá.
TEST_F(HospitalTestFixture, Boundary_Hard) {
    auto p = std::dynamic_pointer_cast<OutPatient>(hospital->findPatient(default_out_id));
    double daily_surcharge = 0.1;
    int visit_days = 30;
    
    // Giả lập bệnh nhân khám liên tục 30 ngày, mỗi ngày cộng thêm một khoản phí rất nhỏ
    for (int i = 1; i <= visit_days; ++i) {
        bool is_weekend = (i % 7 == 6) || (i % 7 == 0);
        double actual_fee = is_weekend ? (daily_surcharge * 1.5) : daily_surcharge;
        p->addVisit("Routine Day " + std::to_string(i), actual_fee);
    }
    
    // Logic test tự tính lại tổng tiền (Base là 150.0)
    double expected_bill = 150.0;
    for (int i = 1; i <= visit_days; ++i) {
        expected_bill += 150.0 + ((i % 7 == 6 || i % 7 == 0) ? 0.15 : 0.1);
    }
    
    // Rootcause: Số 0.1 không thể biểu diễn tuyệt đối trong hệ nhị phân. Cộng dồn 30 lần sẽ 
    // làm lệch giá trị thực tế ở phần thập phân (ví dụ: 153.00000000000003 != 153.0).
    // Fix: Dùng EXPECT_NEAR(p->getBill(), expected_bill, 0.0001); thay vì EXPECT_EQ.
    EXPECT_EQ(p->getBill(), expected_bill); 
}

// ============================================================================
// 5. MEMORY OVERRUN (Buffer Overflow)
// ============================================================================

// [Dễ] Tràn mảng do lặp lớn hơn size.
TEST_F(HospitalTestFixture, MemOverrun_Easy) {
    int active_ids[2];
    auto patients = hospital->getAllPatients();
    // Rootcause: <= làm ghi tràn mảng tĩnh.
    // Fix: i < patients.size()
    for(size_t i = 0; i <= patients.size(); ++i) { 
        if(i < 2) active_ids[i] = patients[i]->getID();
    }
    EXPECT_EQ(active_ids[0], default_in_id);
}

// [Trung bình] Lỗi tràn vùng nhớ thông qua iterator thao tác sai với offset tính toán.
TEST_F(HospitalTestFixture, MemOverrun_Medium) {
    auto patients = hospital->getAllPatients();
    auto it = patients.begin();
    
    // Business logic: muốn lấy bệnh nhân ở vị trí 3/4 của danh sách
    size_t total_count = patients.size();
    size_t target_offset = total_count + 1; // Tính nhầm offset
    
    if (target_offset > 0) {
        std::advance(it, target_offset); 
    }
    
    std::string found_name = "";
    // Rootcause: target_offset vượt quá patients.size(). std::advance đẩy iterator qua khỏi end().
    // Khi dereference (*it) sẽ gây lỗi Memory Overrun.
    // Fix: Đảm bảo offset < total_count.
    if (it != patients.end()) { 
        found_name = (*it)->getName();
    }
    
    EXPECT_NE(found_name, "Unknown"); 
}

// [Khó] Tràn vector do reserve sai cách phối hợp std::copy\_if trong pipeline dữ liệu.
TEST_F(HospitalTestFixture, MemOverrun_Hard) {
    auto all_patients = hospital->getAllPatients();
    std::vector<std::shared_ptr<Patient>> active_patients;
    
    // Trainee muốn tối ưu bộ nhớ nên dùng reserve trước khi copy
    active_patients.reserve(all_patients.size()); 
    
    auto is_active_pred = [](const std::shared_ptr<Patient>& p) {
        return p->isActive() && p->getAge() > 10;
    };
    
    // Rootcause: reserve() chỉ tăng capacity, không thay đổi size (vẫn là 0). 
    // std::copy_if yêu cầu iterator đích phải trỏ tới mảng đã được cấp phát ĐỦ SIZE, 
    // ghi vào mảng size 0 sẽ làm tràn bộ nhớ (buffer overrun).
    // Fix: Dùng std::back_inserter(active_patients) thay cho active_patients.begin().
    std::copy_if(all_patients.begin(), all_patients.end(), 
                 active_patients.begin(), is_active_pred);
                 
    EXPECT_GE(active_patients.size(), 1);
}

// ============================================================================
// 6. TIMEOUT (Infinite Loops)
// ============================================================================

// [Dễ] Điều kiện lặp kiểm tra sai biến.
TEST_F(HospitalTestFixture, Timeout_Easy) {
    int days = 0;
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    // Rootcause: Không gọi p->extendStay(1) nên getDaysAdmitted() không đổi.
    // Fix: Cập nhật state trong vòng lặp.
    while (p->getDaysAdmitted() < 5) { days++; }
    EXPECT_EQ(days, 3);
}

// [Trung bình] Vòng lặp bị treo do container bị sửa đổi ngầm trong quá trình lặp.
TEST_F(HospitalTestFixture, Timeout_Medium) {
    int max_capacity = 100;
    
    // Vòng lặp này có vẻ an toàn vì duyệt theo index
    for (size_t i = 0; i < hospital->getAllPatients().size(); ++i) {
        auto current_patient = hospital->getAllPatients()[i];
        
        // Nếu tìm thấy bệnh nhân nhỏ tuổi, ưu tiên đăng ký thêm 1 người thân vào viện
        if (current_patient->getAge() < 20 && hospital->getTotalPatientCount() < max_capacity) {
            hospital->registerOutPatient("Relative of " + current_patient->getName(), 40, 50.0);
        }
    }
    
    // Rootcause: Việc đăng ký người thân làm tăng size() của mảng. Lệnh for đánh giá lại size() liên tục.
    // Luffy (19 tuổi) thỏa mãn điều kiện, đẻ ra 1 Relative. Relative này chạy tới cuối sẽ không đẻ nữa, 
    // nhưng do điều kiện dừng liên tục dời ra xa nên vòng lặp kéo dài cho tới khi max_capacity. Nếu bỏ max_capacity sẽ lặp vô hạn.
    // Dù có max_capacity, đây vẫn là dạng Timeout do logic duyệt mảng bị phá vỡ.
    // Fix: Khóa size lại (size_t init_size = ...size(); for(.. i < init_size;)).
    EXPECT_EQ(hospital->getTotalPatientCount(), 3);
}

// [Khó] Lặp vô hạn (Livelock) do điều kiện thoát không bao giờ đạt chuẩn.
TEST_F(HospitalTestFixture, Timeout_Hard) {
    auto inpatient = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    double target_revenue = 2000.0;
    
    auto performDailyBilling = [&]() {
        inpatient->addProcedure("Daily Care", 50.0);
        if (inpatient->getProcedureCost() > 300.0) {
            // Logic giảm trừ nếu phí procedure quá cao (vô tình làm giảm tổng bill)
            inpatient->addProcedure("Discount", -50.0); 
        }
    };
    
    // Rootcause: Mỗi lần thêm phí 50, nếu vượt 300 lại bị trừ 50. Do đó tổng phí procedure 
    // cứ lặp lại ở quanh mức 300. Tổng getBill() sẽ không bao giờ đạt được target_revenue (2000.0), 
    // gây ra vòng lặp vô hạn (Livelock).
    // Fix: Cần xem lại nghiệp vụ thiết kế logic Discount, hoặc thêm biến đếm (fail-safe) break vòng lặp.
    while (inpatient->getBill() < target_revenue) {
        performDailyBilling();
    }
    
    EXPECT_GE(inpatient->getBill(), target_revenue);
}

// ============================================================================
// 7. MULTI DECLARED (Shadowing / Macro / GTest Structure Errors)
// ============================================================================

// [Dễ] Multi Declared do define macro.
#define admitInPatient(name, age, room, rate) printf("Admitted")
TEST_F(HospitalTestFixture, MultiDecl_Easy) {
    // Rootcause: Macro đè lên method class.
    // Fix: Xóa define.
    int id = hospital->admitInPatient("Nami", 20, "V2", 200.0); 
    EXPECT_GT(id, 0);
}
#undef admitInPatient

// [Trung bình] Định nghĩa lại một struct nội bộ (ODR Violation).
/* BỎ COMMENT ĐỂ THẤY LỖI
struct VisitRecord {
    std::string alternative_diagnosis; 
    double alternative_fee;
};
*/
// Rootcause: Struct VisitRecord đã nằm trong Hospital.hpp. Định nghĩa lại gây lỗi biên dịch.
// Fix: Bỏ phần struct định nghĩa lại.
TEST_F(HospitalTestFixture, MultiDecl_Medium) {
    EXPECT_TRUE(true);
}

// [Khó] Variable Shadowing làm sụp đổ logic test kẹp trong block try-catch.
TEST_F(HospitalTestFixture, MultiDecl_Hard) {
    int target_age = 0;
    std::string target_name = "";
    bool parse_success = false;
    
    auto extractData = [&]() {
        try {
            auto p = hospital->findPatient(default_in_id);
            if (!p) throw std::runtime_error("Patient not found");
            
            // Trainee khai báo lại biến cục bộ thay vì gán giá trị cho biến ở scope ngoài
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
    
    // Rootcause: `int target_age` và `std::string target_name` trong khối try đã che khuất (shadow) 
    // biến được mong đợi ở bên ngoài. Biến ngoài không bao giờ đổi giá trị.
    // Fix: Bỏ kiểu dữ liệu, chỉ viết `target_age = p->getAge();`
    EXPECT_TRUE(parse_success);
    EXPECT_EQ(target_age, 19); 
    EXPECT_EQ(target_name, "Luffy");
}