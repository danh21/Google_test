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
// 1. TEST ISOLATION / FLAKY TESTS (Rò rỉ trạng thái toàn cục)
// ============================================================================

// Biến global giả lập config của môi trường test
static double global_discount_rate = 1.0; 

// [Dễ] Thay đổi global state nhưng không phục hồi (Teardown).
TEST_F(HospitalTestFixture_2, Isolation_Easy) {
    // Rootcause: Đổi global state để test nhánh này, nhưng không reset lại. 
    // Các test case chạy sau sẽ bị ảnh hưởng ngầm (Flaky test).
    // Fix: Lưu giá trị cũ và khôi phục nó ở cuối hàm, hoặc chuyển vào class Fixture.
    global_discount_rate = 0.5; 
    EXPECT_EQ(global_discount_rate, 0.5);
}

// [Trung bình] Rò rỉ file state do hardcode tên file ghi log.
TEST_F(HospitalTestFixture_2, Isolation_Medium) {
    auto p = hospital->findPatient(default_in_id);
    std::string report_file = "test_report.txt"; // Hardcode chung cho mọi test
    
    std::ofstream out(report_file, std::ios::app);
    out << p->getSummary() << "\n";
    out.close();
    
    std::ifstream in(report_file);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    
    // Rootcause: Nếu test này chạy 2 lần (hoặc có test khác cũng ghi vào test_report.txt),
    // file sẽ phình to ra và size luôn lớn hơn mức mong đợi ban đầu.
    // Fix: Mỗi test case phải dùng một tên file duy nhất (vd: dùng tên test case làm tiền tố), 
    // hoặc phải xóa file trong hàm TearDown().
    bool has_data = content.length() > 10;
    EXPECT_TRUE(has_data);
}

// [Khó] Cache ẩn trong biến static cục bộ của hàm helper.
void processPatientData(std::shared_ptr<Patient> p, std::vector<std::string>& output) {
    static std::vector<int> processed_ids; // Bộ nhớ đệm giữ state giữa các lần gọi
    
    if (std::find(processed_ids.begin(), processed_ids.end(), p->getID()) == processed_ids.end()) {
        output.push_back(p->getName() + " Processed");
        processed_ids.push_back(p->getID());
    }
}

TEST_F(HospitalTestFixture_2, Isolation_Hard) {
    auto p = hospital->findPatient(default_in_id);
    std::vector<std::string> results;
    
    // Chim mồi: Vòng lặp phức tạp để sinh data
    for (int i = 0; i < 5; ++i) {
        if (p->isActive() && p->getAge() > 0) {
            processPatientData(p, results);
        }
    }
    
    // Rootcause: processed_ids là static. Test này chạy lần đầu tiên sẽ EXPECT_EQ = 1.
    // Nếu framework chạy lại suite này (retry on failure), processed_ids đã có sẵn ID của Luffy, 
    // hàm processPatientData không push_back nữa -> results.size() = 0 -> Test Fail ngẫu nhiên.
    // Fix: Xóa từ khóa `static` trong hàm helper, hoặc truyền cache như một tham số (Dependency Injection).
    EXPECT_EQ(results.size(), 1); 
}

// ============================================================================
// 2. ASSERTION MISUSE (False Positives - So sánh sai bản chất)
// ============================================================================

// [Dễ] So sánh con trỏ chuỗi kiểu C thay vì nội dung.
TEST_F(HospitalTestFixture_2, AssertMisuse_Easy) {
    const char* expected_type = "InPatient";
    std::string actual_type = hospital->findPatient(default_in_id)->getType();
    
    // Rootcause: EXPECT_EQ với một bên là con trỏ char*, một bên là c_str() sẽ so sánh 2 
    // địa chỉ bộ nhớ khác nhau (fail logic nhưng báo lỗi rất khó hiểu).
    // Fix: Dùng EXPECT_STREQ hoặc EXPECT_EQ(actual_type, std::string(expected_type));
    EXPECT_NE(actual_type.c_str(), expected_type); 
}

// [Trung bình] So sánh địa chỉ của shared_ptr thay vì giá trị bên trong.
TEST_F(HospitalTestFixture_2, AssertMisuse_Medium) {
    // Tester muốn kiểm tra xem tìm kiếm bằng ID và tìm kiếm bằng Tên có ra cùng kết quả không
    auto patient_by_id = hospital->findPatient(default_in_id);
    auto patient_by_name = hospital->getPatientsByName("Luffy").front();
    
    // Thực hiện vài thao tác update
    patient_by_id->addPrescription("Meat", "1 ton", 1, "Sanji");
    
    // Rootcause: GTest sẽ so sánh 2 object shared_ptr. Do class Patient không override toán tử ==, 
    // nó sẽ so sánh địa chỉ bộ nhớ. Test này pass không phải vì nội dung giống nhau, 
    // mà vì chúng trỏ chung 1 vùng nhớ. Nếu nghiệp vụ yêu cầu clone 2 object độc lập, test này sẽ không phát hiện được.
    // Fix: So sánh từng trường dữ liệu (EXPECT_EQ(p1->getID(), p2->getID()), ...).
    EXPECT_EQ(patient_by_id, patient_by_name); 
}

// [Khó] EXPECT_TRUE nuốt trọn biểu thức phức tạp, mất thông tin debug.
TEST_F(HospitalTestFixture_2, AssertMisuse_Hard) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    
    p->addProcedure("Scan", 100.0);
    p->addProcedure("Surgery", 500.0);
    p->extendStay(5);
    
    bool is_valid_bill = (p->getBill() > 0.0);
    bool is_valid_days = (p->getDaysAdmitted() == 7);
    bool has_procedures = (p->getProcedureCount() == 2);
    
    // Rootcause: Khi test này fail (giả sử do is_valid_days sai), GTest chỉ in ra: 
    // "Value of: is_valid_bill && is_valid_days && has_procedures / Actual: false / Expected: true".
    // Tester hoàn toàn mù tịt không biết điều kiện nào trong 3 cái kia bị sai.
    // Fix: Tách ra thành 3 lệnh EXPECT_TRUE riêng biệt.
    EXPECT_TRUE(is_valid_bill && is_valid_days && has_procedures);
}

// ============================================================================
// 3. EXCEPTION SILENCING (Bưng bít lỗi - False Negatives)
// ============================================================================

// [Dễ] Catch tất cả nhưng quên fail.
TEST_F(HospitalTestFixture_2, ExceptionSilencing_Easy) {
    try {
        // Cố tình truyền ID âm để kích hoạt bug
        auto p = hospital->findPatient(-1); 
        p->getName(); 
    } catch (...) {
        // Rootcause: Có lỗi xảy ra, văng exception, nhảy vào catch nhưng không làm gì cả. 
        // GTest đánh giá test case này PASS. Lỗi bị bưng bít.
        // Fix: Thêm lệnh FAIL() << "Unexpected exception thrown"; vào trong block catch.
    }
}

// [Trung bình] Dùng EXPECT_ANY_THROW ẩn giấu logic sai.
TEST_F(HospitalTestFixture_2, ExceptionSilencing_Medium) {
    auto p = hospital->findPatient(default_out_id);
    
    // Chim mồi: Setup data hợp lệ
    std::string valid_diagnosis = "Flu";
    double valid_fee = 50.0;
    
    auto executeAction = [&]() {
        auto op = std::dynamic_pointer_cast<OutPatient>(p);
        // Trainee vô tình code gõ nhầm nullptr (mô phỏng bug)
        std::shared_ptr<OutPatient> null_op = nullptr; 
        null_op->addVisit(valid_diagnosis, valid_fee); 
    };
    
    // Rootcause: Đoạn code trong lambda văng exception do SegFault (hoặc bad_access) 
    // vì `null_op` là nullptr. EXPECT_ANY_THROW thấy có văng exception nên đánh giá PASS.
    // Thực chất nghiệp vụ lẽ ra không được văng exception (do data hợp lệ).
    // Fix: Dùng EXPECT_NO_THROW hoặc EXPECT_THROW với đúng loại Exception mong đợi (std::runtime_error).
    EXPECT_ANY_THROW(executeAction());
}

// [Khó] Test Helper nuốt exception trả về default value.
int getPatientAgeSafe(Hospital* h, int id) {
    try {
        return h->findPatient(id)->getAge();
    } catch (const std::exception& e) {
        // Log ẩn và trả về giá trị mặc định, bưng bít lỗi
        return 0; 
    }
}

TEST_F(HospitalTestFixture_2, ExceptionSilencing_Hard) {
    // ID 999 không tồn tại, findPatient trả về nullptr
    // Truy xuất ->getAge() văng SegFault hoặc exception tùy OS
    int age = getPatientAgeSafe(hospital.get(), 999);
    
    // Xử lý bù đắp logic bên ngoài
    int current_year = 2026;
    int estimated_birth_year = current_year - age;
    
    // Rootcause: age bị ép về 0 do helper nuốt lỗi. estimated_birth_year = 2026. 
    // Test case đi kiểm tra "nếu không tìm thấy thì là trẻ sơ sinh (sinh năm 2026)".
    // Lỗi hệ thống nghiêm trọng (truy cập nullptr) bị biến thành một "feature" hợp lệ trong mắt GTest.
    // Fix: Xóa khối try-catch trong helper, để exception nổi lên tới GTest, hoặc helper phải return std::optional.
    EXPECT_EQ(estimated_birth_year, 2026);
}

// ============================================================================
// 4. TEST RESOURCE LEAKS (Rò rỉ tài nguyên do viết Test Code ẩu)
// ============================================================================

// [Dễ] Quên delete raw pointer cấp phát trong Test.
TEST_F(HospitalTestFixture_2, ResourceLeak_Easy) {
    // Rootcause: Test dùng `new` để tạo biến tạm kiểm tra logic nhưng quên `delete`.
    // Nếu chạy cùng AddressSanitizer (ASan), build CI/CD sẽ fail vì memory leak.
    // Fix: Dùng std::unique_ptr<int> hoặc khai báo biến trên stack (int expected_bill = 150).
    int* expected_bill = new int(150); 
    
    auto p = hospital->findPatient(default_out_id);
    EXPECT_EQ(p->getBill(), *expected_bill);
}

// [Trung bình] Leak file descriptor (handle) khi verify data.
TEST_F(HospitalTestFixture_2, ResourceLeak_Medium) {
    std::string dump_path = "patient_dump.tmp";
    
    // Ghi file
    std::ofstream out(dump_path);
    out << hospital->findPatient(default_in_id)->getSummary();
    out.close();
    
    // Verify file nội dung bằng C-style (để làm khó trainee)
    FILE* file = fopen(dump_path.c_str(), "r");
    ASSERT_NE(file, nullptr);
    
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) != nullptr) {
        std::string line(buffer);
        EXPECT_TRUE(line.find("ID") != std::string::npos);
    }
    
    // Rootcause: File descriptor `file` không bao giờ được đóng lại bằng fclose().
    // Trong một Test Suite lớn chạy hàng nghìn test case, lỗi này sẽ vắt kiệt file handles của OS.
    // Fix: Thêm fclose(file); trước khi kết thúc hàm test.
}

// [Khó] Cyclic Reference của std::shared_ptr do Mock cấu trúc trong test.
struct TestObserver {
    std::shared_ptr<Patient> observed_patient;
    std::shared_ptr<TestObserver> self_ref; // Chim mồi
};

TEST_F(HospitalTestFixture_2, ResourceLeak_Hard) {
    auto p = hospital->findPatient(default_in_id);
    
    // Trainee muốn tạo một observer để theo dõi vòng đời đối tượng
    auto observer = std::make_shared<TestObserver>();
    observer->observed_patient = p;
    
    // Rootcause: Trainee gán self_ref = chính nó (Cyclic Reference). 
    // Ref-count của observer sẽ không bao giờ về 0 khi rời khỏi hàm, gây memory leak âm thầm.
    // Smart pointer không giải quyết được cyclic reference, phải dùng std::weak_ptr.
    // Fix: Xóa dòng self_ref = observer hoặc đổi kiểu self_ref thành weak_ptr.
    observer->self_ref = observer; 
    
    EXPECT_EQ(observer->observed_patient->getAge(), 19);
}

// ============================================================================
// 5. VACUOUS TRUTH (Vòng lặp/Logic rỗng, test auto-pass)
// ============================================================================

// [Dễ] Điều kiện vòng lặp sai ngay từ đầu.
TEST_F(HospitalTestFixture_2, VacuousTruth_Easy) {
    auto patients = hospital->getAllPatients();
    
    // Rootcause: i < 0 luôn sai (i = 0). Vòng lặp chứa ASSERT không bao giờ chạy.
    // Test case chớp nhoáng báo PASS khiến trainee tưởng code đúng.
    // Fix: i < patients.size()
    for (size_t i = 0; i < 0; ++i) { 
        EXPECT_GT(patients[i]->getAge(), 0);
    }
}

// [Trung bình] Lọc sai điều kiện làm mảng đích rỗng, skip kiểm tra.
TEST_F(HospitalTestFixture_2, VacuousTruth_Medium) {
    auto all_patients = hospital->getAllPatients();
    std::vector<std::shared_ptr<Patient>> seniors;
    
    // Trích xuất bệnh nhân trên 60 tuổi
    for (auto p : all_patients) {
        if (p->getAge() >= 60) seniors.push_back(p);
    }
    
    // Chim mồi: Các thao tác tính toán phức tạp
    double total_senior_bill = 0;
    for (auto s : seniors) {
        total_senior_bill += s->getBill();
        // Kiểm tra nghiệp vụ quan trọng: Senior luôn được giảm giá
        EXPECT_LE(s->getBill(), 100.0); 
    }
    
    // Rootcause: Trong hàm SetUp() chỉ có Luffy(19) và Zoro(21). Mảng seniors rỗng (size = 0).
    // Vòng lặp kiểm tra (chứa EXPECT_LE) không được thực thi lần nào. Test auto-pass.
    // Fix: Cần thêm dữ liệu test h.admitInPatient("Old", 65...) hoặc check ASSERT_FALSE(seniors.empty());
}

// [Khó] Return sớm phá vỡ luồng kiểm tra (Silent test exit).
TEST_F(HospitalTestFixture_2, VacuousTruth_Hard) {
    auto p = hospital->findPatient(default_in_id);
    
    auto validatePatientRecord = [&](std::shared_ptr<Patient> patient) {
        if (!patient->isActive()) return; // Thoát sớm nếu inactive
        
        // Tester gõ nhầm điều kiện thành != 19 thay vì == 19
        if (patient->getAge() != 19) {
            // Rootcause: Khi sai độ tuổi, thay vì báo lỗi (FAIL), tester lại dùng `return`. 
            // Lambda kết thúc, chương trình tiếp tục chạy và coi như test PASS hoàn hảo.
            // Fix: Sửa return thành ADD_FAILURE() << "Wrong age"; return;
            return; 
        }
        
        EXPECT_EQ(patient->getName(), "Luffy");
    };
    
    // Thay đổi độ tuổi để trigger luồng rẽ nhánh sai
    hospital->admitInPatient("Sanji", 20, "V3", 100);
    validatePatientRecord(hospital->findPatient(3));
}

// ============================================================================
// 6. THREADING PITFALLS IN GTEST (Macro an toàn luồng)
// ============================================================================

// [Dễ] Detach thread không kịp chạy assertion.
TEST_F(HospitalTestFixture_2, ThreadPitfall_Easy) {
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // Rootcause: Do dùng t.detach(), main thread kết thúc và báo PASS trước khi 
        // thread này kịp thức dậy và kiểm tra. Nếu đoạn EXPECT này fail, nó sẽ fail ở 
        // background hoặc crash luôn GTest Runner một cách khó hiểu.
        // Fix: Dùng t.join() thay vì t.detach().
        EXPECT_EQ(hospital->getTotalPatientCount(), 999); 
    });
    t.detach(); 
}

// [Trung bình] Dùng ASSERT_* trong helper function sai kiểu trả về.
// Cấu trúc kiểm tra:
void checkHospitalName(Hospital* h) {
    // Rootcause: ASSERT_EQ thực chất là macro: { if(false) return; }. 
    // Vì hàm này trả về void, nó chỉ ngắt (return) hàm checkHospitalName, 
    // rồi tiếp tục chạy test case chính ở bên dưới, không hề ngắt toàn bộ Test Case như kỳ vọng.
    // Fix: Đối với hàm trả về void, chỉ được dùng EXPECT_*. Hoặc đổi hàm thành kiểu ::testing::AssertionResult.
    ASSERT_EQ(h->getHospitalName(), "Wrong Name"); 
    
    // Dòng dưới không được chạy, nhưng test chính vẫn tiếp tục.
    h->registerOutPatient("Test", 20, 10); 
}

TEST_F(HospitalTestFixture_2, ThreadPitfall_Medium) {
    checkHospitalName(hospital.get());
    
    // Dòng này vẫn được chạy và pass, làm trainee hoang mang
    EXPECT_EQ(hospital->getTotalPatientCount(), 2); 
}

// [Khó] Lỗi đồng bộ hóa khi nhiều thread cùng ghi vào luồng lỗi của GTest.
TEST_F(HospitalTestFixture_2, ThreadPitfall_Hard) {
    std::vector<std::thread> workers;
    
    for (int i = 0; i < 10; ++i) {
        workers.emplace_back([&, i]() {
            auto new_id = hospital->admitInPatient("Clone", 20, "1", 10.0);
            
            // Rootcause: EXPECT_GT không thread-safe nếu có hằng hà sa số luồng cùng đập 
            // lỗi vào console output của GTest (nếu nó fail). GTest có thể bị deadlock nội bộ 
            // hoặc output console rác nhòe nhoẹt. 
            // Fix: Lưu kết quả kiểm tra vào biến bool an toàn (atomic), sau khi join() xong 
            // mới mang biến bool đó ra ngoài thread chính để EXPECT_TRUE.
            EXPECT_GT(new_id, 0); 
        });
    }
    
    for (auto& w : workers) { w.join(); }
}

// ============================================================================
// 7. TEST LOGIC MISCALCULATION (Sai số trong tự tính Expected Value)
// ============================================================================

// [Dễ] Lỗi chia số nguyên trong Test code.
TEST_F(HospitalTestFixture_2, LogicCalc_Easy) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    
    // Tester tự tính tiền mong đợi bằng cách giảm giá 50%
    // Rootcause: (1 / 2) là phép chia số nguyên trong C++, kết quả là 0. expected_discount_bill = 0.
    // C++ làm tròn xuống, test sẽ fail vì bill thực tế > 0.
    // Fix: Sửa thành (1.0 / 2.0).
    double expected_discount_bill = (1 / 2) * (500.0 * 2); 
    
    EXPECT_EQ(p->getBill() * 0.5, expected_discount_bill);
}

// [Trung bình] Tính sai độ tuổi do nhầm lẫn logic xử lý thời gian.
TEST_F(HospitalTestFixture_2, LogicCalc_Medium) {
    auto p = hospital->findPatient(default_out_id);
    int current_year = 2026;
    
    // Giả lập logic lấy tuổi thông qua ngày sinh
    std::string mock_dob_from_db = "2005-12-31"; 
    int birth_year = std::stoi(mock_dob_from_db.substr(0, 4));
    
    // Tính số tháng tuổi để quy đổi (Chim mồi: dùng logic cồng kềnh)
    int age_in_months = (current_year - birth_year) * 12;
    int expected_age = age_in_months / 12;
    
    // Thêm 1 năm bù trừ (sai lệch logic của tester)
    // Rootcause: Tester tự thêm 1 vào expected_age do tưởng tính tuổi mụ, trong khi source code 
    // không hề có logic này (Zoro 21 tuổi). expected_age trở thành 22. Test sẽ đánh fail source code đúng.
    // Fix: Xóa dòng cộng 1. Expected value phải thuần túy theo đúng rule của Requirements.
    expected_age += 1; 
    
    EXPECT_EQ(p->getAge(), expected_age);
}

// [Khó] Overflow kiểu dữ liệu khi giả lập tải thời gian dài.
TEST_F(HospitalTestFixture_2, LogicCalc_Hard) {
    auto p = std::dynamic_pointer_cast<InPatient>(hospital->findPatient(default_in_id));
    
    // Giả lập bệnh nhân này ở lại viện trong vòng 50 năm để test hóa đơn
    int years = 50;
    int days_per_year = 365;
    int daily_rate = 500;
    
    // Cập nhật state (hàm extendStay an toàn do code source dùng int)
    p->extendStay(years * days_per_year);
    
    // Rootcause: Tester dùng kiểu `int` nhân nhau (50 * 365 * 500 = 9,125,000). 
    // Nếu thêm procedure hoặc rate cao (VD daily_rate = 500,000), kết quả nhân int sẽ bị tràn (Integer Overflow), 
    // biến thành số âm trước khi được ép kiểu ngầm định lên double của expected_total_bill.
    // Source code tính bằng double ngay từ đầu nên đúng. Test code tự tính sai nên đánh fail.
    // Fix: Ép kiểu double ngay từ lúc tính toán: double expected = double(years * days) * rate;
    double expected_total_bill = (years * days_per_year * daily_rate) + (2 * 500); // Base ban đầu là 2 ngày
    
    EXPECT_DOUBLE_EQ(p->getBill(), expected_total_bill);
}