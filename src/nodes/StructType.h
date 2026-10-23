#pragma once
#include <string>
#include <utility>
#include <vector>

// Port of src/nodes/core/StructType.js at the pinned revision. A struct type names a group of
// members the shader can pass around as one value. The one flag it carries says whether it
// describes output rather than input, which a shader's interface needs to tell apart.
class StructType {
  public:
    using Member = std::pair<std::string, std::string>;

    StructType(std::string name, std::vector<Member> members)
        : name_(std::move(name)), members_(std::move(members)) {}

    // The type's own name.
    const std::string& name() const { return name_; }

    // Its members, each a name and a type, in the order they were given.
    const std::vector<Member>& members() const { return members_; }

    // Whether this describes an output rather than an input. A fresh type is an input.
    bool output() const { return output_; }
    void setOutput(bool value) { output_ = value; }

    // The member with a given name, or null when there is none.
    const Member* member(const std::string& memberName) const {
        for (const auto& entry : members_) {
            if (entry.first == memberName) return &entry;
        }
        return nullptr;
    }

    // How many members there are.
    std::size_t memberCount() const { return members_.size(); }

  private:
    std::string name_;
    std::vector<Member> members_;
    // The flag starts clear, as the source's constructor leaves it.
    bool output_ = false;
};
