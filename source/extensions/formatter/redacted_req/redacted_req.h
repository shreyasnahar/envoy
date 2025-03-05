#pragma once

#include <string>

#include "envoy/config/typed_config.h"
#include "envoy/registry/registry.h"
#include "envoy/http/header_map.h"

#include "source/common/formatter/substitution_format_utility.h"
#include "source/common/formatter/substitution_formatter.h"
#include "source/extensions/filters/common/expr/evaluator.h"

#include "absl/container/btree_map.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace Formatter {

class RedactedReq : public ::Envoy::Formatter::FormatterProvider {
public:
  RedactedReq(const std::string& main_header, const std::string& alternative_header, const std::vector<std::string> key_words,
                  absl::optional<size_t> max_length);

  absl::optional<std::string>
  formatWithContext(const Envoy::Formatter::HttpFormatterContext& context,
                    const StreamInfo::StreamInfo&) const override;
  ProtobufWkt::Value formatValueWithContext(const Envoy::Formatter::HttpFormatterContext& context,
                                            const StreamInfo::StreamInfo&) const override;

private:
  const Http::HeaderEntry* findHeader(const Http::HeaderMap& headers) const;
  const std::string redact(const Http::HeaderEntry& header) const;


  Http::LowerCaseString main_header_;
  Http::LowerCaseString alternative_header_;
  std::vector<std::string> key_words_;
  absl::optional<size_t> max_length_;
};

class RedactedReqCommandParser : public ::Envoy::Formatter::CommandParser {
public:
  RedactedReqCommandParser() = default;
  ::Envoy::Formatter::FormatterProviderPtr parse(absl::string_view command,
                                                 absl::string_view subcommand,
                                                 absl::optional<size_t> max_length) const override;

private:
  void parseSubcommand(absl::string_view subcommand, std::string& header, std::string& keys) const;
  const std::vector<std::string> parseKeys(const std::string &s, char delim) const;
};

struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        for (size_t i = 0; i < a.size() && i < b.size(); ++i) {
            char c1 = std::tolower(static_cast<unsigned char>(a[i]));
            char c2 = std::tolower(static_cast<unsigned char>(b[i]));
            if (c1 != c2) {
                return c1 < c2;
            }
        }
        return a.size() < b.size();
    }
};

class QueryParamsMultiCaseInsensitive {
private:
  absl::btree_map<std::string, std::vector<std::string>, CaseInsensitiveCompare> data_;

public:
  void remove(absl::string_view key);
  void add(absl::string_view key, absl::string_view value);
  void overwrite(absl::string_view key, absl::string_view value);
  std::string toString() const;
  std::string replaceQueryString(const Http::HeaderString& path) const;
  absl::optional<std::string> getFirstValue(absl::string_view key) const;

  const absl::btree_map<std::string, std::vector<std::string>, CaseInsensitiveCompare>& data() const { return data_; }

  static QueryParamsMultiCaseInsensitive parseParameters(absl::string_view data, size_t start, bool decode_params);
  static QueryParamsMultiCaseInsensitive parseQueryString(absl::string_view url);
  static QueryParamsMultiCaseInsensitive parseAndDecodeQueryString(absl::string_view url);
};

} // namespace Formatter
} // namespace Extensions
} // namespace Envoy