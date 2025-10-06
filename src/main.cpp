#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

struct Adjustment {
    std::string description;
    double amount{};
};

struct PayrollRecord {
    int month{};
    int year{};
    double regularHours{};
    double overtimeHours{};
    std::vector<Adjustment> allowances;
    std::vector<Adjustment> deductions;
    double grossPay{};
    double netPay{};
    bool finalized{false};
};

struct Employee {
    int id{};
    std::string fullName;
    std::string position;
    double monthlySalary{};
    double hourlyRate{};
    double taxRate{}; // income tax as 0.0 - 1.0
};

class PayrollSystem {
public:
    void addEmployee(const Employee &employee) {
        employees_.push_back(employee);
        std::cout << "Працівника додано: " << employee.fullName << " (ID " << employee.id << ")\n";
    }

    void listEmployees() const {
        if (employees_.empty()) {
            std::cout << "Список працівників порожній.\n";
            return;
        }
        std::cout << "\n=== Працівники ===\n";
        for (const auto &employee : employees_) {
            std::cout << "ID: " << employee.id << ", " << employee.fullName << " — "
                      << employee.position << ", оклад: " << employee.monthlySalary
                      << " грн, погодинна ставка: " << employee.hourlyRate
                      << " грн, ставка ПДФО: " << employee.taxRate * 100 << "%\n";
        }
        std::cout << std::endl;
    }

    void recordWorkHours(int employeeId, int month, int year, double regularHours, double overtimeHours) {
        auto &record = ensureRecord(employeeId, month, year);
        record.regularHours += regularHours;
        record.overtimeHours += overtimeHours;
        std::cout << "Години роботи оновлено для ID " << employeeId << " за "
                  << month << "/" << year << "\n";
    }

    void addAllowance(int employeeId, int month, int year, const Adjustment &allowance) {
        auto &record = ensureRecord(employeeId, month, year);
        record.allowances.push_back(allowance);
        std::cout << "Надбавку додано.\n";
    }

    void addDeduction(int employeeId, int month, int year, const Adjustment &deduction) {
        auto &record = ensureRecord(employeeId, month, year);
        record.deductions.push_back(deduction);
        std::cout << "Утримання додано.\n";
    }

    void runPayroll(int month, int year) {
        std::cout << "\n=== Формування відомості за " << month << "/" << year << " ===\n";
        for (const auto &employee : employees_) {
            auto recordOpt = findRecord(employee.id, month, year);
            if (!recordOpt) {
                std::cout << "Для працівника " << employee.fullName
                          << " (ID " << employee.id << ") немає даних.\n";
                continue;
            }

            auto &record = *recordOpt;
            if (record.finalized) {
                std::cout << "Відомість уже сформовано для " << employee.fullName << ".\n";
                continue;
            }

            const double allowancesSum = accumulateAmounts(record.allowances);
            const double deductionsSum = accumulateAmounts(record.deductions);
            const double regularPay = record.regularHours * employee.hourlyRate;
            const double overtimePay = record.overtimeHours * employee.hourlyRate * 1.5;
            const double gross = employee.monthlySalary + regularPay + overtimePay + allowancesSum;
            const double tax = gross * employee.taxRate;
            const double net = gross - tax - deductionsSum;

            record.grossPay = gross;
            record.netPay = net;
            record.finalized = true;

            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Працівник: " << employee.fullName << "\n"
                      << "  Базовий оклад: " << employee.monthlySalary << " грн\n"
                      << "  Години: " << record.regularHours << " (понаднормові " << record.overtimeHours << ")\n"
                      << "  Надбавки: " << allowancesSum << " грн\n"
                      << "  Утримання: " << deductionsSum << " грн\n"
                      << "  Податок: " << tax << " грн\n"
                      << "  До виплати: " << record.netPay << " грн\n\n";
        }
    }

    void printPayslip(int employeeId, int month, int year) const {
        const Employee *employee = findEmployee(employeeId);
        if (!employee) {
            std::cout << "Працівника з таким ID не існує.\n";
            return;
        }

        auto recordOpt = findRecordConst(employeeId, month, year);
        if (!recordOpt || !recordOpt->finalized) {
            std::cout << "Платіжна відомість для цього періоду не сформована.\n";
            return;
        }

        const PayrollRecord &record = *recordOpt;
        const double allowancesSum = accumulateAmounts(record.allowances);
        const double deductionsSum = accumulateAmounts(record.deductions);
        const double tax = (employee->monthlySalary + record.regularHours * employee->hourlyRate +
                            record.overtimeHours * employee->hourlyRate * 1.5 + allowancesSum) *
                           employee->taxRate;

        std::cout << "\n=== Розрахунковий лист (" << month << "/" << year << ") ===\n";
        std::cout << "Працівник: " << employee->fullName << " — " << employee->position << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Базовий оклад: " << employee->monthlySalary << " грн\n";
        std::cout << "Години: " << record.regularHours << ", понаднормові: " << record.overtimeHours << "\n";
        std::cout << "Надбавки: " << allowancesSum << " грн\n";
        std::cout << "Утримання: " << deductionsSum << " грн\n";
        std::cout << "ПДФО: " << tax << " грн\n";
        std::cout << "До виплати: " << record.netPay << " грн\n";
    }

private:
    PayrollRecord &ensureRecord(int employeeId, int month, int year) {
        auto &records = payroll_[employeeId];
        auto it = std::find_if(records.begin(), records.end(), [&](const PayrollRecord &r) {
            return r.month == month && r.year == year;
        });

        if (it == records.end()) {
            records.push_back(PayrollRecord{month, year});
            return records.back();
        }
        return *it;
    }

    PayrollRecord *findRecord(int employeeId, int month, int year) {
        auto it = payroll_.find(employeeId);
        if (it == payroll_.end()) {
            return nullptr;
        }
        auto &records = it->second;
        auto recIt = std::find_if(records.begin(), records.end(), [&](const PayrollRecord &r) {
            return r.month == month && r.year == year;
        });
        if (recIt == records.end()) {
            return nullptr;
        }
        return &(*recIt);
    }

    const PayrollRecord *findRecordConst(int employeeId, int month, int year) const {
        auto it = payroll_.find(employeeId);
        if (it == payroll_.end()) {
            return nullptr;
        }
        const auto &records = it->second;
        auto recIt = std::find_if(records.begin(), records.end(), [&](const PayrollRecord &r) {
            return r.month == month && r.year == year;
        });
        if (recIt == records.end()) {
            return nullptr;
        }
        return &(*recIt);
    }

    const Employee *findEmployee(int employeeId) const {
        auto it = std::find_if(employees_.begin(), employees_.end(), [&](const Employee &employee) {
            return employee.id == employeeId;
        });
        if (it == employees_.end()) {
            return nullptr;
        }
        return &(*it);
    }

    PayrollRecord *findRecord(int employeeId, int month, int year) const {
        return const_cast<PayrollRecord *>(findRecordConst(employeeId, month, year));
    }

    static double accumulateAmounts(const std::vector<Adjustment> &adjustments) {
        return std::accumulate(adjustments.begin(), adjustments.end(), 0.0,
                               [](double sum, const Adjustment &adj) { return sum + adj.amount; });
    }

    std::vector<Employee> employees_;
    std::map<int, std::vector<PayrollRecord>> payroll_;
};

namespace {
void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int readInt(const std::string &prompt) {
    int value{};
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            clearInput();
            return value;
        }
        std::cout << "Некоректне значення, спробуйте ще раз.\n";
        clearInput();
    }
}

double readDouble(const std::string &prompt) {
    double value{};
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            clearInput();
            return value;
        }
        std::cout << "Некоректне значення, спробуйте ще раз.\n";
        clearInput();
    }
}

std::string readLine(const std::string &prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}
}

void printMenu() {
    std::cout << "\n=== Автоматизована система обліку заробітної плати ===\n"
              << "1. Додати працівника\n"
              << "2. Показати працівників\n"
              << "3. Внести відпрацьовані години\n"
              << "4. Додати надбавку\n"
              << "5. Додати утримання\n"
              << "6. Сформувати платіжну відомість\n"
              << "7. Надрукувати розрахунковий лист\n"
              << "0. Вихід\n";
}

Adjustment createAdjustment(const std::string &type) {
    Adjustment adjustment;
    adjustment.description = readLine("Опис " + type + ": ");
    adjustment.amount = readDouble("Сума "+ type + ": ");
    return adjustment;
}

int main() {
    PayrollSystem payrollSystem;

    // Декілька працівників за замовчуванням
    payrollSystem.addEmployee({1, "Іван Петренко", "Бухгалтер", 18000.0, 150.0, 0.195});
    payrollSystem.addEmployee({2, "Олена Сидоренко", "Розробник", 32000.0, 250.0, 0.195});

    bool running = true;
    while (running) {
        printMenu();
        int choice = readInt("Оберіть дію: ");

        switch (choice) {
        case 1: {
            Employee employee;
            employee.id = readInt("ID працівника: ");
            employee.fullName = readLine("ПІБ: ");
            employee.position = readLine("Посада: ");
            employee.monthlySalary = readDouble("Місячний оклад (грн): ");
            employee.hourlyRate = readDouble("Погодинна ставка (грн): ");
            employee.taxRate = readDouble("Ставка ПДФО (наприклад 0.195): ");
            payrollSystem.addEmployee(employee);
            break;
        }
        case 2:
            payrollSystem.listEmployees();
            break;
        case 3: {
            int id = readInt("ID працівника: ");
            int month = readInt("Місяць (1-12): ");
            int year = readInt("Рік: ");
            double regularHours = readDouble("Звичайні години: ");
            double overtimeHours = readDouble("Понаднормові години: ");
            payrollSystem.recordWorkHours(id, month, year, regularHours, overtimeHours);
            break;
        }
        case 4: {
            int id = readInt("ID працівника: ");
            int month = readInt("Місяць (1-12): ");
            int year = readInt("Рік: ");
            payrollSystem.addAllowance(id, month, year, createAdjustment("надбавки"));
            break;
        }
        case 5: {
            int id = readInt("ID працівника: ");
            int month = readInt("Місяць (1-12): ");
            int year = readInt("Рік: ");
            payrollSystem.addDeduction(id, month, year, createAdjustment("утримання"));
            break;
        }
        case 6: {
            int month = readInt("Місяць (1-12): ");
            int year = readInt("Рік: ");
            payrollSystem.runPayroll(month, year);
            break;
        }
        case 7: {
            int id = readInt("ID працівника: ");
            int month = readInt("Місяць (1-12): ");
            int year = readInt("Рік: ");
            payrollSystem.printPayslip(id, month, year);
            break;
        }
        case 0:
            running = false;
            break;
        default:
            std::cout << "Невідома команда.\n";
            break;
        }
    }

    std::cout << "Завершення роботи системи.\n";
    return 0;
}
