/**
 * Hospital.cpp
 *
 * Implementation of Patient, InPatient, OutPatient, and Hospital.
 * No main() — designed as a reusable library component.
 */

#include "Hospital.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <numeric>

// ─── File-scope helper ────────────────────────────────────────────────────────

static std::string todayDate() {
    std::time_t t = std::time(nullptr);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return std::string(buf);
}

// ════════════════════════════════════════════════════════════════════════════
// Patient (abstract base)
// ════════════════════════════════════════════════════════════════════════════

Patient::Patient(int id, const std::string& name, int age)
    : patient_id(id), name(name), age(age), is_active(true)
{
    if (id <= 0)
        throw std::invalid_argument("Patient ID must be positive.");
    if (name.empty())
        throw std::invalid_argument("Patient name must not be empty.");
    if (age < 0 || age > 150)
        throw std::invalid_argument("Patient age must be between 0 and 150.");
}

Patient::~Patient() {}

// ── Getters ──────────────────────────────────────────────────────────────────
int         Patient::getID()                 const { return patient_id; }
std::string Patient::getName()               const { return name; }
int         Patient::getAge()                const { return age; }
bool        Patient::isActive()              const { return is_active; }
int         Patient::getPrescriptionCount()  const {
    return static_cast<int>(prescriptions.size());
}
const std::vector<Prescription>& Patient::getPrescriptions() const {
    return prescriptions;
}

// ── Setter ───────────────────────────────────────────────────────────────────
void Patient::setName(const std::string& new_name) {
    if (new_name.empty())
        throw std::invalid_argument("Patient name must not be empty.");
    name = new_name;
}

// ── Prescription management ───────────────────────────────────────────────────
void Patient::addPrescription(const std::string& medicine,
                              const std::string& dosage,
                              int                duration_days,
                              const std::string& prescribed_by) {
    if (medicine.empty())
        throw std::invalid_argument("Medicine name must not be empty.");
    if (duration_days <= 0)
        throw std::invalid_argument("Duration must be at least 1 day.");
    prescriptions.push_back({medicine, dosage, duration_days, prescribed_by});
}

void Patient::clearPrescriptions() {
    prescriptions.clear();
}

// ── getSummary (base) ─────────────────────────────────────────────────────────
std::string Patient::getSummary() const {
    std::ostringstream oss;
    oss << "ID     : " << patient_id                        << "\n"
        << "Name   : " << name                              << "\n"
        << "Age    : " << age                               << "\n"
        << "Type   : " << getType()                         << "\n"
        << "Status : " << (is_active ? "Active" : "Inactive") << "\n"
        << "Rx     : " << prescriptions.size() << " prescription(s)\n"
        << "Bill   : " << std::fixed << std::setprecision(2) << getBill();
    return oss.str();
}

// ════════════════════════════════════════════════════════════════════════════
// InPatient
// ════════════════════════════════════════════════════════════════════════════

InPatient::InPatient(int id, const std::string& name, int age,
                     const std::string& room_number,
                     double daily_rate,
                     int    days_admitted)
    : Patient(id, name, age),
      room_number(room_number),
      daily_rate(daily_rate),
      days_admitted(days_admitted)
{
    if (room_number.empty())
        throw std::invalid_argument("Room number must not be empty.");
    if (daily_rate < 0.0)
        throw std::invalid_argument("Daily rate cannot be negative.");
    if (days_admitted < 0)
        throw std::invalid_argument("Days admitted cannot be negative.");
}

// ── Getters ───────────────────────────────────────────────────────────────────
std::string InPatient::getRoomNumber()    const { return room_number; }
double      InPatient::getDailyRate()     const { return daily_rate; }
int         InPatient::getDaysAdmitted()  const { return days_admitted; }
int         InPatient::getProcedureCount() const {
    return static_cast<int>(procedures.size());
}
const std::vector<Procedure>& InPatient::getProcedures() const {
    return procedures;
}

double InPatient::getProcedureCost() const {
    double total = 0.0;
    for (const auto& p : procedures) total += p.cost;
    return total;
}

// ── addProcedure ──────────────────────────────────────────────────────────────
void InPatient::addProcedure(const std::string& proc_name, double cost) {
    if (!is_active)
        throw std::runtime_error("Cannot add procedure to a discharged patient.");
    if (proc_name.empty())
        throw std::invalid_argument("Procedure name must not be empty.");
    if (cost < 0.0)
        throw std::invalid_argument("Procedure cost cannot be negative.");
    procedures.push_back({proc_name, cost});
}

// ── extendStay ────────────────────────────────────────────────────────────────
void InPatient::extendStay(int extra_days) {
    if (!is_active)
        throw std::runtime_error("Cannot extend stay of a discharged patient.");
    if (extra_days <= 0)
        throw std::invalid_argument("Extension must be at least 1 day.");
    days_admitted += extra_days;
}

// ── discharge ─────────────────────────────────────────────────────────────────
void InPatient::discharge() {
    if (!is_active)
        throw std::runtime_error("Patient is already discharged.");
    is_active = false;
}

// ── getBill ───────────────────────────────────────────────────────────────────
double InPatient::getBill() const {
    return static_cast<double>(days_admitted) * daily_rate + getProcedureCost();
}

std::string InPatient::getType() const { return "InPatient"; }

std::string InPatient::getSummary() const {
    std::ostringstream oss;
    oss << Patient::getSummary()                                      << "\n"
        << "Room   : " << room_number                                 << "\n"
        << "Rate   : " << std::fixed << std::setprecision(2)
                       << daily_rate << "/day\n"
        << "Days   : " << days_admitted                               << "\n"
        << "Procs  : " << procedures.size() << " procedure(s)";
    return oss.str();
}

// ════════════════════════════════════════════════════════════════════════════
// OutPatient
// ════════════════════════════════════════════════════════════════════════════

OutPatient::OutPatient(int id, const std::string& name, int age,
                       double base_consultation_fee)
    : Patient(id, name, age),
      base_consultation_fee(base_consultation_fee)
{
    if (base_consultation_fee < 0.0)
        throw std::invalid_argument("Base consultation fee cannot be negative.");
}

// ── Getters ───────────────────────────────────────────────────────────────────
double OutPatient::getBaseConsultationFee() const {
    return base_consultation_fee;
}
int OutPatient::getVisitCount() const {
    return static_cast<int>(visits.size());
}
const std::vector<VisitRecord>& OutPatient::getVisits() const {
    return visits;
}

double OutPatient::getTotalFees() const {
    double total = 0.0;
    for (const auto& v : visits) total += v.fee;
    return total;
}

// ── addVisit ──────────────────────────────────────────────────────────────────
void OutPatient::addVisit(const std::string& diagnosis, double additional_fee) {
    if (!is_active)
        throw std::runtime_error("Cannot add visit to an inactive patient.");
    if (diagnosis.empty())
        throw std::invalid_argument("Diagnosis must not be empty.");
    if (additional_fee < 0.0)
        throw std::invalid_argument("Additional fee cannot be negative.");
    double total_visit_fee = base_consultation_fee + additional_fee;
    visits.push_back({diagnosis, total_visit_fee, todayDate()});
}

// ── deactivate ────────────────────────────────────────────────────────────────
void OutPatient::deactivate() {
    if (!is_active)
        throw std::runtime_error("Patient is already inactive.");
    is_active = false;
}

// ── getBill ───────────────────────────────────────────────────────────────────
double      OutPatient::getBill()    const { return getTotalFees(); }
std::string OutPatient::getType()    const { return "OutPatient"; }

std::string OutPatient::getSummary() const {
    std::ostringstream oss;
    oss << Patient::getSummary()                                      << "\n"
        << "BaseFee: " << std::fixed << std::setprecision(2)
                       << base_consultation_fee                       << "\n"
        << "Visits : " << visits.size();
    return oss.str();
}

// ════════════════════════════════════════════════════════════════════════════
// Hospital
// ════════════════════════════════════════════════════════════════════════════

Hospital::Hospital(const std::string& name) : hospital_name(name), next_id(1) {
    if (name.empty())
        throw std::invalid_argument("Hospital name must not be empty.");
}

// ── admitInPatient ────────────────────────────────────────────────────────────
int Hospital::admitInPatient(const std::string& name, int age,
                             const std::string& room_number,
                             double daily_rate, int initial_days) {
    int id = next_id++;
    patients.push_back(
        std::make_shared<InPatient>(id, name, age, room_number,
                                    daily_rate, initial_days));
    return id;
}

// ── registerOutPatient ────────────────────────────────────────────────────────
int Hospital::registerOutPatient(const std::string& name, int age,
                                 double base_consultation_fee) {
    int id = next_id++;
    patients.push_back(
        std::make_shared<OutPatient>(id, name, age, base_consultation_fee));
    return id;
}

// ── findPatient — returns nullptr when ID is absent ───────────────────────────
std::shared_ptr<Patient> Hospital::findPatient(int id) const {
    auto it = std::find_if(patients.begin(), patients.end(),
        [id](const std::shared_ptr<Patient>& p){ return p->getID() == id; });
    return (it != patients.end()) ? *it : nullptr;
}

// ── dischargePatient ──────────────────────────────────────────────────────────
bool Hospital::dischargePatient(int id) {
    auto p = findPatient(id);
    if (!p || !p->isActive()) return false;

    if (auto ip = std::dynamic_pointer_cast<InPatient>(p)) {
        ip->discharge();
    } else if (auto op = std::dynamic_pointer_cast<OutPatient>(p)) {
        op->deactivate();
    }
    return true;
}

// ── getPatientsByName ─────────────────────────────────────────────────────────
std::vector<std::shared_ptr<Patient>>
Hospital::getPatientsByName(const std::string& name) const {
    std::vector<std::shared_ptr<Patient>> result;
    for (const auto& p : patients)
        if (p->getName() == name) result.push_back(p);
    return result;
}

// ── getTotalRevenue — sum of getBill() for ALL registered patients ─────────────
double Hospital::getTotalRevenue() const {
    double total = 0.0;
    for (const auto& p : patients) total += p->getBill();
    return total;
}

std::string Hospital::getHospitalName()       const { return hospital_name; }
int         Hospital::getTotalPatientCount()   const { return static_cast<int>(patients.size()); }
int         Hospital::getActivePatientCount()  const {
    return static_cast<int>(
        std::count_if(patients.begin(), patients.end(),
            [](const std::shared_ptr<Patient>& p){ return p->isActive(); }));
}

const std::vector<std::shared_ptr<Patient>>& Hospital::getAllPatients() const {
    return patients;
}
