#include <gtest/gtest.h>
#include "Hospital.hpp"
#include <thread>
#include <string_view>
#include <functional>
#include <cstring>
#include <numeric>

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
// 1. SEGMENTATION FAULT (Dereferencing Null/Invalid Pointers)
// ============================================================================

// [Dễ] Dereference trực tiếp một raw pointer bị null.
TEST(HospitalTest, SegFault_Easy) {
    Hospital h("City Hospital");
    auto p = h.findPatient(999); 
    
    // Rootcause: Tìm một ID không tồn tại sẽ trả về nullptr. Test case không kiểm tra null.
    // Fix: Thêm ASSERT_NE(p, nullptr); trước khi gọi p->getName().
    EXPECT_EQ(p->getName(), "John Doe"); 
}

// [Trung bình] Ép kiểu sai (Bad Cast) bị ẩn giấu trong một chuỗi thao tác.
TEST(HospitalTest, SegFault_Medium) {
    Hospital h("City Hospital");
    int in_id = h.admitInPatient("Bob", 40, "101", 200.0);
    int out_id = h.registerOutPatient("Alice", 25, 100.0);
    
    // Giả lập logic lấy bệnh nhân ngẫu nhiên hoặc theo điều kiện nghiệp vụ
    auto target_patient = h.findPatient(out_id); 
    
    // Làm một số thao tác không liên quan để đánh lạc hướng
    ASSERT_TRUE(target_patient->isActive());
    EXPECT_EQ(target_patient->getAge(), 25);

    // Rootcause: target_patient đang là OutPatient, ép kiểu sang InPatient sẽ ra nullptr.
    // Fix: Kiểm tra if (ip) trước khi dùng, hoặc dùng hàm ảo (virtual) thay vì ép kiểu.
    auto ip = std::dynamic_pointer_cast<InPatient>(target_patient);
    ip->addProcedure("X-Ray", 50.0); 
    EXPECT_EQ(ip->getProcedureCount(), 1);
}

// [Khó] Sử dụng con trỏ chưa khởi tạo trong logic rẽ nhánh phức tạp.
TEST(HospitalTest, SegFault_Hard) {
    Hospital h("City Hospital");
    h.admitInPatient("Bob", 40, "101", 200.0);
    
    Patient* ptr; 
    bool is_vip_ward_full = (h.getTotalPatientCount() > 100);
    bool is_emergency = false;

    // Logic rẽ nhánh khiến tester tưởng ptr chắc chắn được gán
    if (is_vip_ward_full) {
        ptr = h.findPatient(1).get();
    } else if (is_emergency) {
        ptr = nullptr; // Fallback logic
    }
    
    // Rootcause: Cả 2 nhánh trên đều false, ptr mang giá trị rác (uninitialized pointer).
    // Fix: Khởi tạo Patient* ptr = nullptr; ngay từ đầu và check if(ptr) ở dưới.
    EXPECT_EQ(ptr->getAge(), 40); 
}

// ============================================================================
// 2. USE AFTER FREE (Dangling Pointers / References)
// ============================================================================

// [Dễ] Dangling reference do vector reallocation.
TEST(HospitalTest, UAF_Easy) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("John", 30, "A1", 100.0);
    auto p = std::dynamic_pointer_cast<InPatient>(h.findPatient(id));
    
    p->addProcedure("Blood Test", 50.0);
    const auto& proc = p->getProcedures()[0]; 
    
    for(int i = 0; i < 100; ++i) {
        p->addProcedure("Test " + std::to_string(i), 10.0);
    }
    // Rootcause: vector procedures cấp phát lại vùng nhớ, biến proc trỏ vào vùng nhớ cũ bị giải phóng.
    // Fix: Trích xuất tên (std::string name = p->getProcedures()[0].name;) thay vì giữ reference.
    EXPECT_EQ(proc.name, "Blood Test"); 
}

// [Trung bình] Vòng đời của biến tạm (Temporary object lifetime) bọc qua hàm ẩn.
TEST(HospitalTest, UAF_Medium) {
    Hospital h("City Hospital");
    int id = h.registerOutPatient("Alice", 25, 150.0);
    
    auto getPatientNameCStr = [&](int pid) -> const char* {
        // Trả về c_str() của một rvalue string
        return h.findPatient(pid)->getName().c_str(); 
    };
    
    const char* namePtr = getPatientNameCStr(id);
    
    // Làm một số thao tác tốn thời gian/bộ nhớ ở đây để vùng nhớ rác bị ghi đè rõ ràng hơn
    std::vector<int> dummy(1000, 1); 
    
    // Rootcause: getPatientNameCStr trả về con trỏ từ chuỗi tạm thời (bị hủy ngay khi rời hàm).
    // Fix: Sửa hàm getPatientNameCStr để trả về std::string, giữ chuỗi sống thay vì con trỏ char*.
    EXPECT_STREQ(namePtr, "Alice"); 
}

// [Khó] Lambda capture by reference trỏ ra ngoài scope ẩn trong callback.
TEST(HospitalTest, UAF_Hard) {
    std::function<double()> calculateFinalBill;
    Hospital h("City Hospital");

    // Tạo một scope giả lập môi trường xử lý event/callback
    {
        int id = h.admitInPatient("Bob", 40, "A3", 200.0, 5);
        auto p = h.findPatient(id);
        
        // Tester cài cắm logic tính bill phức tạp
        p->addPrescription("Aspirin", "1 viên", 5, "Dr. Smith");
        
        calculateFinalBill = [&p]() { 
            double discount = p->getPrescriptionCount() > 0 ? 0.9 : 1.0;
            return p->getBill() * discount; 
        }; 
    } // p (shared_ptr) hết scope và bị hủy

    // Rootcause: Lambda capture shared_ptr p bằng reference [&p]. Khi gọi hàm, p đã bị hủy.
    // Fix: Capture by value `[p]()` để lambda giữ ownership của shared_ptr.
    EXPECT_GT(calculateFinalBill(), 0.0); 
}

// ============================================================================
// 3. DATA CORRUPTION (Undefined Behavior / Race Conditions)
// ============================================================================

// [Dễ] Khởi tạo thiếu biến đầu vào.
TEST(HospitalTest, DataCorrupt_Easy) {
    int patient_age; 
    Hospital h("City Hospital");
    
    // Rootcause: patient_age mang giá trị rác. Nếu rác rớt vào khoản < 0 hoặc > 150, test sẽ fail.
    // Fix: Khởi tạo int patient_age = 25;
    EXPECT_NO_THROW(h.admitInPatient("John", patient_age, "101", 100.0)); 
}

// [Trung bình] Race condition làm hỏng vector với vỏ bọc xử lý song song giả tạo.
TEST(HospitalTest, DataCorrupt_Medium) {
    Hospital h("City Hospital");
    std::mutex mtx; // Đặt mutex để đánh lừa trainee rằng code đã thread-safe
    
    auto processBatch = [&]() {
        for(int i = 0; i < 1000; ++i) {
            // Cố tình lock ở biến đếm hoặc thao tác rác thay vì bảo vệ hàm của Hospital
            std::lock_guard<std::mutex> lock(mtx);
            h.registerOutPatient("Patient", 30, 50.0);
        }
    };
    
    std::thread t1(processBatch);
    std::thread t2(processBatch);
    t1.join(); 
    t2.join();
    
    // Rootcause: Lock_guard bảo vệ scope vô nghĩa, bên trong h.registerOutPatient (chứa vector) vẫn bị race condition.
    // Fix: Class Hospital cần cài đặt mutex nội bộ cho các thao tác ghi dữ liệu.
    EXPECT_EQ(h.getTotalPatientCount(), 2000); 
}

// [Khó] Shared Pointer Aliasing (Hiểu lầm về copy vs share).
TEST(HospitalTest, DataCorrupt_Hard) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("Charlie", 50, "A1", 200.0);
    
    auto original = h.findPatient(id);
    
    // Tester nghĩ rẳng getAllPatients trả về bản sao độc lập của bệnh nhân để verify logic cũ
    auto cached_list = h.getAllPatients();
    auto snapshot_patient = cached_list[0]; 
    
    original->discharge(); // Thao tác trên thực thể "gốc"
    
    // Rootcause: snapshot_patient và original trỏ chung một vùng nhớ (shared_ptr). Đổi 1 bên là đổi tất cả.
    // Fix: Trạng thái không thể snapshot kiểu này. Cần tạo hàm deep copy (clone) nếu muốn so sánh lịch sử.
    EXPECT_TRUE(snapshot_patient->isActive()); 
}

// ============================================================================
// 4. BOUNDARY (Off-by-one / Limits / Precision)
// ============================================================================

// [Dễ] Sai lầm biên điều kiện tối đa.
TEST(HospitalTest, Boundary_Easy) {
    Hospital h("City Hospital");
    
    // Rootcause: Độ tuổi giới hạn trong Hospital.cpp là <= 150. Truyền 151 sẽ văng exception.
    // Fix: Dùng EXPECT_THROW(..., std::invalid_argument);
    EXPECT_NO_THROW(h.admitInPatient("Old Man", 151, "101", 100.0));
}

// [Trung bình] Lỗi Off-by-one ẩn trong pointer arithmetic hoặc reverse loop.
TEST(HospitalTest, Boundary_Medium) {
    Hospital h("City Hospital");
    h.admitInPatient("A", 10, "1", 10.0);
    h.admitInPatient("B", 20, "2", 20.0);
    auto all = h.getAllPatients();
    
    int active_count = 0;
    // Đi lùi từ cuối mảng nhưng nhầm toán tử điều kiện dừng
    for (int i = all.size(); i >= 0; --i) { 
        if (all[i]->isActive()) active_count++;
    }
    
    // Rootcause: Bắt đầu từ all.size() là nằm ngoài biên của mảng (Out of bounds).
    // Fix: Bắt đầu từ int i = all.size() - 1;
    EXPECT_EQ(active_count, 2);
}

// [Khó] Lỗi so sánh dấu phẩy động sau nhiều phép toán tích lũy.
TEST(HospitalTest, Boundary_Hard) {
    Hospital h("City Hospital");
    int id = h.registerOutPatient("John", 30, 0.0);
    auto p = std::dynamic_pointer_cast<OutPatient>(h.findPatient(id));
    
    // Thêm 10 procedure có giá 0.1
    for(int i = 0; i < 10; ++i) {
        p->addVisit("Checkup", 0.1);
    }
    
    // Rootcause: 0.1 cộng 10 lần trong hệ nhị phân sinh ra sai số (vd: 0.9999999999999999).
    // Fix: Thay vì EXPECT_EQ, hãy dùng EXPECT_DOUBLE_EQ(p->getBill(), 1.0);
    EXPECT_EQ(p->getBill(), 1.0); 
}

// ============================================================================
// 5. MEMORY OVERRUN (Buffer Overflow)
// ============================================================================

// [Dễ] Ghi tràn mảng tĩnh (Stack overflow).
TEST(HospitalTest, MemOverrun_Easy) {
    int ids[2];
    Hospital h("City Hospital");
    ids[0] = h.admitInPatient("A", 10, "1", 10.0);
    ids[1] = h.admitInPatient("B", 20, "2", 20.0);
    
    // Rootcause: Mảng chỉ chứa được 2 phần tử, ghi vào index 2 làm tràn stack.
    // Fix: Khai báo int ids[3]; hoặc std::vector<int> ids;.
    ids[2] = h.admitInPatient("C", 30, "3", 30.0); 
    EXPECT_EQ(h.getTotalPatientCount(), 3);
}

// [Trung bình] C-style string copy tràn bộ đệm ẩn giấu qua hàm ghép chuỗi.
TEST(HospitalTest, MemOverrun_Medium) {
    Hospital h("City Hospital");
    std::string prefix = "Dr_";
    std::string doctor_name = "Alexander_The_Great";
    
    int id = h.admitInPatient("Patient", 20, "1", 10.0);
    h.findPatient(id)->addPrescription("Meds", "1", 1, prefix + doctor_name);
    
    char buffer[16]; // Buffer nhỏ cố ý
    
    // Rootcause: Tổng độ dài chuỗi prescription by lớn hơn 16 char. strcpy sẽ ghi đè vùng nhớ lân cận.
    // Fix: Dùng std::string thay cho mảng char, hoặc dùng strncpy.
    strcpy(buffer, h.findPatient(id)->getPrescriptions()[0].prescribed_by.c_str()); 
    
    EXPECT_EQ(buffer[0], 'D');
}

// [Khó] Tràn vector do reserve sai cách phối hợp std::copy.
TEST(HospitalTest, MemOverrun_Hard) {
    Hospital h("City Hospital");
    h.admitInPatient("A", 10, "1", 10.0);
    h.admitInPatient("B", 20, "2", 20.0);
    
    std::vector<std::shared_ptr<Patient>> copy_vec;
    copy_vec.reserve(h.getTotalPatientCount()); // reserve chỉ cấp capacity, size vẫn = 0
    
    auto patients = h.getAllPatients();
    
    // Rootcause: std::copy yêu cầu size của destination phải đủ chứa. Ở đây size = 0, ghi vào sẽ văng Out of range.
    // Fix: Đổi .reserve thành .resize, hoặc dùng std::back_inserter(copy_vec).
    std::copy(patients.begin(), patients.end(), copy_vec.begin()); 
    EXPECT_EQ(copy_vec.size(), 2);
}

// ============================================================================
// 6. TIMEOUT (Infinite Loops)
// ============================================================================

// [Dễ] Quên cập nhật biến đếm vòng lặp.
TEST(HospitalTest, Timeout_Easy) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("A", 10, "1", 10.0);
    auto p = std::dynamic_pointer_cast<InPatient>(h.findPatient(id));
    
    // Rootcause: Thiếu lệnh cập nhật ngày bên trong vòng lặp.
    // Fix: Thêm p->extendStay(1); vào thân vòng lặp.
    while(p->getDaysAdmitted() < 10) {
        // Missing state update
    }
    EXPECT_EQ(p->getBill(), 100.0);
}

// [Trung bình] Lỗi cập nhật iterator bị block bởi logic continue ẩn.
TEST(HospitalTest, Timeout_Medium) {
    Hospital h("City Hospital");
    h.admitInPatient("A", 10, "1", 10.0);
    h.admitInPatient("B", 20, "2", 20.0);
    auto all = h.getAllPatients();
    
    for (auto it = all.begin(); it != all.end(); ) {
        bool should_skip = (*it)->getAge() < 15;
        
        // Rootcause: Khi should_skip == true, vòng lặp nhảy lên đầu mà không tăng iterator ++it -> kẹt ở A mãi.
        // Fix: Đưa ++it; lên trước lệnh continue, hoặc dùng vòng for có điều kiện ++it ở header.
        if (should_skip) continue; 
        
        EXPECT_TRUE((*it)->isActive());
        ++it;
    }
}

// [Khó] Lặp vô hạn do thao tác sai đối tượng được che đậy bằng tính toán phức tạp.
TEST(HospitalTest, Timeout_Hard) {
    Hospital h("City Hospital");
    h.admitInPatient("Target", 10, "1", 10.0);
    
    // Hàm giả lập tìm ID theo tên (viết sai logic trả về ID cố định)
    auto findIdByName = [&](const std::string& name) {
        auto list = h.getPatientsByName(name);
        return list.empty() ? 999 : list[0]->getID() + 100; // Cố tình cộng 100 gây sai ID
    };

    // Rootcause: target_id bị tính sai (101 thay vì 1). Hàm dischargePatient(101) trả về false, active count không bao giờ giảm.
    // Fix: Sửa logic của findIdByName để trả đúng ID thật của bệnh nhân.
    int target_id = findIdByName("Target");
    while (h.getActivePatientCount() > 0) {
        h.dischargePatient(target_id); 
    }
    EXPECT_EQ(h.getActivePatientCount(), 0);
}

// ============================================================================
// 7. MULTI DECLARED (Shadowing / Macro / GTest Structure Errors)
// ============================================================================
// [Trung bình] Lẫn lộn giữa TEST và TEST_F làm hỏng scope phân giải macro.
class MyHospitalTest : public ::testing::Test {
protected:
    Hospital h{"TestHosp"};
};

TEST_F(MyHospitalTest, BasicOp) { EXPECT_EQ(h.getHospitalName(), "TestHosp"); }

// Decoy: Trainee bị phân tâm bởi các hàm helper giữa chừng
void dummyHelper() {}

// Rootcause: Lớp MyHospitalTest đã là fixture dùng cho TEST_F, dùng macro TEST sẽ tạo conflict định nghĩa của GTest.
// Fix: Đổi TEST thành TEST_F.
TEST(MyHospitalTest, AdvancedOp) { 
    EXPECT_TRUE(true); 
}

// [Khó] Lỗi "Multi Declared" thông qua Variable Shadowing phá hỏng logic validation.
TEST(HospitalTest, MultiDecl_Hard) {
    Hospital h("City Hospital");
    int id = h.admitInPatient("John", 30, "101", 100.0);
    
    bool patient_found = false; // Declaration outer
    
    auto checkPatient = [&]() {
        if (auto p = h.findPatient(id)) {
            // Rootcause: Trainee khai báo lại kiểu `bool` khiến compiler hiểu đây là một biến cục bộ mới 
            // che khuất (shadow) biến bên ngoài. Biến bên ngoài không bao giờ được cập nhật.
            // Fix: Xóa chữ `bool` ở dòng dưới, chỉ ghi `patient_found = true;`
            bool patient_found = true; // Declaration inner (Shadowing)
        }
    };
    
    checkPatient();
    EXPECT_TRUE(patient_found); 
}