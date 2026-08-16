#include "forgekv/db.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

namespace {

void PrintUsage(std::string_view program_name) {
  std::cerr << "Usage:\n"
            << "  " << program_name << " <database-dir> put <key> <value>\n"
            << "  " << program_name << " <database-dir> get <key>\n"
            << "  " << program_name << " <database-dir> delete <key>\n";
}

int PrintFailure(const forgekv::Status& status) {
  std::cerr << "ForgeKV error: " << status.ToString() << '\n';
  return 1;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 3) {
    PrintUsage(argv[0]);
    return 1;
  }

  const std::filesystem::path directory = argv[1];
  const std::string_view command = argv[2];

  if ((command == "put" && argc != 5) ||
      ((command == "get" || command == "delete") && argc != 4) ||
      (command != "put" && command != "get" && command != "delete")) {
    PrintUsage(argv[0]);
    return 1;
  }

  std::unique_ptr<forgekv::DB> db;
  forgekv::Status status = forgekv::DB::Open(directory, &db);
  if (!status.ok()) {
    return PrintFailure(status);
  }

  if (command == "put") {
    status = db->Put(argv[3], argv[4]);
  } else if (command == "get") {
    std::string value;
    status = db->Get(argv[3], &value);
    if (status.ok()) {
      std::cout << value << '\n';
    }
  } else {
    status = db->Delete(argv[3]);
  }

  const forgekv::Status close_status = db->Close();
  if (!status.ok()) {
    return PrintFailure(status);
  }
  if (!close_status.ok()) {
    return PrintFailure(close_status);
  }

  return 0;
}
