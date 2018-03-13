/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi, <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "ApiClient.hpp"

namespace oatpp { namespace web { namespace client {

ApiClient::PathSegment ApiClient::parsePathSegment(p_char8 data, v_int32 size, v_int32& position) {
  for(v_int32 i = position; i < size; i++){
    v_char8 a = data[i];
    if(a == '{'){
      auto result = PathSegment(std::string((char*) &data[position], i - position), PathSegment::SEG_PATH);
      position = i;
      return result;
    }
  }
  auto result = PathSegment(std::string((char*) &data[position], size - position), PathSegment::SEG_PATH);
  position = size;
  return result;
}

ApiClient::PathSegment ApiClient::parseVarSegment(p_char8 data, v_int32 size, v_int32& position) {
  for(v_int32 i = position; i < size; i++){
    v_char8 a = data[i];
    if(a == '}'){
      auto result = PathSegment(std::string((char*) &data[position], i - position), PathSegment::SEG_VAR);
      position = i + 1;
      return result;
    }
  }
  auto result = PathSegment(std::string((char*) &data[position], size - position), PathSegment::SEG_VAR);
  position = size;
  return result;
}
  
ApiClient::PathPattern ApiClient::parsePathPattern(p_char8 data, v_int32 size) {
  v_int32 pos = 0;
  PathPattern result;
  while (pos < size) {
    v_char8 a = data[pos];
    if(a == '{') {
      pos ++;
      result.push_back(parseVarSegment(data, size, pos));
    } else {
      result.push_back(parsePathSegment(data, size, pos));
    }
  }
  return result;
}
  
void ApiClient::formatPath(oatpp::data::stream::OutputStream* stream,
                           const PathPattern& pathPattern,
                           const std::shared_ptr<StringToParamMap>& params) {
  
  for (auto it = pathPattern.begin(); it != pathPattern.end(); ++ it) {
    const PathSegment& seg = *it;
    if(seg.type == PathSegment::SEG_PATH) {
      stream->write(seg.text.data(), seg.text.size());
    } else {
      auto key = base::String::createShared((p_char8) seg.text.data(), (v_int32) seg.text.length(), false);
      auto& param = params->get(key, oatpp::data::mapping::type::AbstractSharedWrapper::empty());
      if(param.isNull()){
        OATPP_LOGD(TAG, "Path parameter '%s' not provided in the api call", (const char*) seg.text.c_str());
        throw std::runtime_error("[oatpp::web::client::ApiClient]: Path parameter missing");
      }
      auto value = oatpp::data::mapping::type::primitiveToStr(param);
      stream->data::stream::OutputStream::write(value);
    }
  }
  
}

void ApiClient::addPathQueryParams(oatpp::data::stream::OutputStream* stream,
                                   const std::shared_ptr<StringToParamMap>& params) {
  
  auto curr = params->getFirstEntry();
  if(curr != nullptr) {
    stream->write("?", 1);
    stream->data::stream::OutputStream::write(curr->getKey());
    stream->write("=", 1);
    stream->data::stream::OutputStream::write(oatpp::data::mapping::type::primitiveToStr(curr->getValue()));
    curr = curr->getNext();
    while (curr != nullptr) {
      stream->write("&", 1);
      stream->data::stream::OutputStream::write(curr->getKey());
      stream->write("=", 1);
      stream->data::stream::OutputStream::write(oatpp::data::mapping::type::primitiveToStr(curr->getValue()));
      curr = curr->getNext();
    }
  }
  
}

std::shared_ptr<ApiClient::StringToStringMap> ApiClient::convertParamsMap(const std::shared_ptr<StringToParamMap>& params) {
  
  if(!params){
    return nullptr;
  }
  
  auto result = StringToStringMap::createShared();
  auto curr = params->getFirstEntry();
  
  while (curr != nullptr) {
    result->put(curr->getKey(), oatpp::data::mapping::type::primitiveToStr(curr->getValue()));
    curr = curr->getNext();
  }
  
  return result;
  
}
  
  
}}}