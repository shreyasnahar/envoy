#include "source/extensions/formatter/redacted_req/redacted_req.h"

#include "source/common/config/metadata.h"
#include "source/common/formatter/substitution_formatter.h"
#include "source/common/http/utility.h"
#include "source/common/protobuf/utility.h"


namespace Envoy {
namespace Extensions {
namespace Formatter {

namespace {

void truncate(std::string& str, absl::optional<size_t> max_length) {
  if (!max_length) {
    return;
  }

  str = str.substr(0, max_length.value());
}

} // namespace

RedactedReq::RedactedReq(const std::string& main_header,
                                 const std::string& alternative_header,
                                 const std::vector<std::string> key_words,
                                 absl::optional<size_t> max_length)
    : main_header_(main_header), alternative_header_(alternative_header), key_words_(key_words), max_length_(max_length) {}

absl::optional<std::string>
RedactedReq::formatWithContext(const Envoy::Formatter::HttpFormatterContext& context,
                                   const StreamInfo::StreamInfo&) const {
  const Http::HeaderEntry* header = findHeader(context.requestHeaders());
  if (!header) {
    return absl::nullopt;
  }

  std::string val = redact(*header);
  // std::string val = Http::Utility::stripQueryString(header->value());
  truncate(val, max_length_);

  return val;
 }

ProtobufWkt::Value
RedactedReq::formatValueWithContext(const Envoy::Formatter::HttpFormatterContext& context,
                                        const StreamInfo::StreamInfo&) const {
  const Http::HeaderEntry* header = findHeader(context.requestHeaders());
  if (!header) {
    return ValueUtil::nullValue();
  }

  std::string val = redact(*header);
  truncate(val, max_length_);
  return ValueUtil::stringValue(val);
}

const std::string RedactedReq::redact(const Http::HeaderEntry& header) const {
  QueryParamsMultiCaseInsensitive query_parameters =
      QueryParamsMultiCaseInsensitive::parseQueryString(header.value().getStringView());

  // traverse key_words to redact
  bool redacted = false;
  for (const auto& key_word : key_words_) {
    auto data = query_parameters.getFirstValue(key_word);
    if (data.has_value()) {
      query_parameters.overwrite(key_word, "<redacted>");
      redacted = true;
    }
  }

  //reconstruct request uri if redacted, else extract original request uri
  std::string val;
  if (redacted) {
    val = query_parameters.toString();
  } else {
    val = std::string{header.value().getStringView()};
  }
  return val;
}

const Http::HeaderEntry* RedactedReq::findHeader(const Http::HeaderMap& headers) const {
  const auto header = headers.get(main_header_);

  if (header.empty() && !alternative_header_.get().empty()) {
    const auto alternate_header = headers.get(alternative_header_);
    // TODO(https://github.com/envoyproxy/envoy/issues/13454): Potentially log all header values.
    return alternate_header.empty() ? nullptr : alternate_header[0];
  }

  return header.empty() ? nullptr : header[0];
}

::Envoy::Formatter::FormatterProviderPtr
RedactedReqCommandParser::parse(absl::string_view command, absl::string_view subcommand,
                                    absl::optional<size_t> max_length) const {
  if (command == "REDACTED_REQ") {
    std::string header, keys, main_header, alternative_header;

    parseSubcommand(subcommand, header, keys);
    std::vector<std::string> key_words = parseKeys(keys, '|');
    
    auto parsed_headers = Envoy::Formatter::SubstitutionFormatUtils::parseSubcommandHeaders(header);
    if (!parsed_headers.ok()) {
      return nullptr;
    }
    main_header = std::string(parsed_headers.value().first);
    alternative_header = std::string(parsed_headers.value().second);

    return std::make_unique<RedactedReq>(main_header, alternative_header, key_words, max_length);
  }

  return nullptr;
}

void RedactedReqCommandParser::parseSubcommand(absl::string_view subcommand, std::string& header, std::string& keys) const {
  size_t pos = subcommand.find(',');
  header = std::string(subcommand.substr(0, pos));
  keys = std::string(subcommand.substr(pos + 1));
}

const std::vector<std::string> RedactedReqCommandParser::parseKeys(const std::string &s, char delim) const {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

QueryParamsMultiCaseInsensitive QueryParamsMultiCaseInsensitive::parseQueryString(absl::string_view url) {
  size_t start = url.find('?');
  if (start == std::string::npos) {
    return {};
  }

  start++;
  return QueryParamsMultiCaseInsensitive::parseParameters(url, start, /*decode_params=*/false);
}

QueryParamsMultiCaseInsensitive
QueryParamsMultiCaseInsensitive::parseAndDecodeQueryString(absl::string_view url) {
  size_t start = url.find('?');
  if (start == std::string::npos) {
    return {};
  }

  start++;
  return QueryParamsMultiCaseInsensitive::parseParameters(url, start, /*decode_params=*/true);
}

QueryParamsMultiCaseInsensitive QueryParamsMultiCaseInsensitive::parseParameters(absl::string_view data,
                                                                     size_t start,
                                                                     bool decode_params) {
  QueryParamsMultiCaseInsensitive params;

  while (start < data.size()) {
    size_t end = data.find('&', start);
    if (end == std::string::npos) {
      end = data.size();
    }
    absl::string_view param(data.data() + start, end - start);

    const size_t equal = param.find('=');
    if (equal != std::string::npos) {
      const auto param_name = StringUtil::subspan(data, start, start + equal);
      const auto param_value = StringUtil::subspan(data, start + equal + 1, end);
      params.add(decode_params ? Http::Utility::PercentEncoding::decode(param_name) : param_name,
                 decode_params ? Http::Utility::PercentEncoding::decode(param_value) : param_value);
    } else {
      const auto param_name = StringUtil::subspan(data, start, end);
      params.add(decode_params ? Http::Utility::PercentEncoding::decode(param_name) : param_name, "");
    }

    start = end + 1;
  }

  return params;
}

void QueryParamsMultiCaseInsensitive::remove(absl::string_view key) { this->data_.erase(std::string{key}); }

void QueryParamsMultiCaseInsensitive::add(absl::string_view key, absl::string_view value) {
  auto result = this->data_.emplace(std::string(key), std::vector<std::string>{std::string(value)});
  if (!result.second) {
    result.first->second.push_back(std::string(value));
  }
}

void QueryParamsMultiCaseInsensitive::overwrite(absl::string_view key, absl::string_view value) {
  this->data_[std::string{key}] = std::vector<std::string>{std::string(value)};
}

absl::optional<std::string> QueryParamsMultiCaseInsensitive::getFirstValue(absl::string_view key) const {
  auto it = this->data_.find(std::string{key});
  if (it == this->data_.end()) {
    return std::nullopt;
  }

  return absl::optional<std::string>{it->second.at(0)};
}

std::string QueryParamsMultiCaseInsensitive::replaceQueryString(const Http::HeaderString& path) const {
  std::string new_path{Http::Utility::stripQueryString(path)};

  if (!this->data_.empty()) {
    absl::StrAppend(&new_path, this->toString());
  }

  return new_path;
}

std::string QueryParamsMultiCaseInsensitive::toString() const {
  std::string out;
  std::string delim = "?";
  for (const auto& p : this->data_) {
    for (const auto& v : p.second) {
      absl::StrAppend(&out, delim, p.first, "=", v);
      delim = "&";
    }
  }
  return out;
}

} // namespace Formatter
} // namespace Extensions
} // namespace Envoy