#ifndef HOSPITAL_HPP
#define HOSPITAL_HPP

/**
 * Hospital.hpp
 *
 * Hospital Patient Management System
 *
 * Classes:
 *   Prescription  (struct)   — medicine record attached to any patient
 *   VisitRecord   (struct)   — single outpatient consultation record
 *   Patient       (abstract) — base: id, name, age, prescriptions
 *   InPatient                — admitted patient: room, daily rate, procedures
 *   OutPatient               — walk-in patient: visit history, fees
 *   Hospital                 — registry + operations over all patients
 */

#include <string>
#include <vector>
#include <memory>

// ─── Prescription ─────────────────────────────────────────────────────────────
struct Prescription {
    std::string medicine;
    std::string dosage;
    int         duration_days;
    std::string prescribed_by;
};

// ─── VisitRecord ──────────────────────────────────────────────────────────────
struct VisitRecord {
    std::string diagnosis;
    double      fee;
    std::string date;   // "YYYY-MM-DD" string
};

// ─── Patient (abstract) ───────────────────────────────────────────────────────
class Patient {
protected:
    int                       patient_id;
    std::string               name;
    int                       age;
    bool                      is_active;     // currently under hospital care
    std::vector<Prescription> prescriptions;

public:
    Patient(int id, const std::string& name, int age);
    virtual ~Patient();

    // Getters
    int                               getID()                  const;
    std::string                       getName()                const;
    int                               getAge()                 const;
    bool                              isActive()               const;
    const std::vector<Prescription>&  getPrescriptions()       const;
    int                               getPrescriptionCount()   const;

    // Setter
    void setName(const std::string& new_name);

    // Prescription management
    void addPrescription(const std::string& medicine,
                         const std::string& dosage,
                         int                duration_days,
                         const std::string& prescribed_by);
    void clearPrescriptions();

    // Polymorphic interface
    virtual double      getBill()     const = 0;
    virtual std::string getType()     const = 0;
    virtual std::string getSummary()  const;
};

// ─── InPatient ────────────────────────────────────────────────────────────────
struct Procedure {
    std::string name;
    double      cost;
};

class InPatient : public Patient {
private:
    std::string           room_number;
    double                daily_rate;
    int                   days_admitted;
    std::vector<Procedure> procedures;  // ordered list of performed procedures

public:
    InPatient(int id, const std::string& name, int age,
              const std::string& room_number,
              double daily_rate,
              int    days_admitted = 0);

    // Getters
    std::string                     getRoomNumber()     const;
    double                          getDailyRate()      const;
    int                             getDaysAdmitted()   const;
    const std::vector<Procedure>&   getProcedures()     const;
    int                             getProcedureCount() const;
    double                          getProcedureCost()  const;  // sum of all procedure costs

    // Operations
    void addProcedure(const std::string& name, double cost);
    void extendStay(int extra_days);
    void discharge();   // sets is_active=false; throws if already inactive

    // Overrides
    double      getBill()    const override;   // days × daily_rate + procedure costs
    std::string getType()    const override;
    std::string getSummary() const override;
};

// ─── OutPatient ───────────────────────────────────────────────────────────────
class OutPatient : public Patient {
private:
    double                  base_consultation_fee;
    std::vector<VisitRecord> visits;

public:
    OutPatient(int id, const std::string& name, int age,
               double base_consultation_fee);

    // Getters
    double                          getBaseConsultationFee() const;
    const std::vector<VisitRecord>& getVisits()              const;
    int                             getVisitCount()          const;
    double                          getTotalFees()           const;  // sum of all visit fees

    // Operations
    void addVisit(const std::string& diagnosis, double additional_fee);
    void deactivate();  // sets is_active=false; throws if already inactive

    // Overrides
    double      getBill()    const override;   // == getTotalFees()
    std::string getType()    const override;
    std::string getSummary() const override;
};

// ─── Hospital ─────────────────────────────────────────────────────────────────
class Hospital {
private:
    std::string                            hospital_name;
    std::vector<std::shared_ptr<Patient>>  patients;
    int                                    next_id;

public:
    explicit Hospital(const std::string& name);

    // Factory methods
    int admitInPatient   (const std::string& name, int age,
                          const std::string& room_number,
                          double daily_rate,
                          int    initial_days = 0);

    int registerOutPatient(const std::string& name, int age,
                           double base_consultation_fee);

    // Lookup
    std::shared_ptr<Patient> findPatient(int id) const;  // nullptr if not found

    // Operations
    bool dischargePatient(int id);   // false if not found / already inactive

    // Queries
    std::vector<std::shared_ptr<Patient>> getPatientsByName(
        const std::string& name) const;

    double      getTotalRevenue()       const;  // sum of getBill() for ALL patients
    std::string getHospitalName()       const;
    int         getTotalPatientCount()  const;
    int         getActivePatientCount() const;

    const std::vector<std::shared_ptr<Patient>>& getAllPatients() const;
};

#endif // HOSPITAL_HPP
