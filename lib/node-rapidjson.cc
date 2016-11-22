// addon.cc
#include <node.h>
#include "../deps/rapidjson/include/rapidjson/document.h"
#include "../deps/rapidjson/include/rapidjson/writer.h"
#include "../deps/rapidjson/include/rapidjson/stringbuffer.h"
#include <uv.h>
#include <string>
#include <chrono>
#include <iostream>

using namespace std;

namespace json_tape {

struct Work {
  v8::Persistent<v8::Function> callback;
  uint32_t length;
  char *input;
  rapidjson::Document *document;
  uv_work_t request;
};

void StartJsonTask(uv_work_t *req) {
  Work *work = (Work *)req->data;

  work->document = new rapidjson::Document;

  if (work->document->ParseInsitu(work->input).HasParseError()) {
    fprintf(stderr, "\nError(offset %u): %d\n", 
            (unsigned)work->document->GetErrorOffset(), work->document->GetParseError());
  }
}

v8::Local<v8::Value> ConvertDocument(v8::Isolate *isolate, rapidjson::Value document);

v8::Local<v8::Object> ConvertObject(v8::Isolate *isolate, rapidjson::Value::Object document) {
  v8::Local<v8::Object> result = v8::Object::New(isolate);

  for (rapidjson::Value::MemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
    v8::Local<v8::String> key = v8::String::NewFromOneByte(isolate, (uint8_t *)itr->name.GetString());
    result->Set(key, ConvertDocument(isolate, itr->value.GetObject())); // TODO: don't hardcode getobject
  }

  return result;
}

v8::Local<v8::Array> ConvertArray(v8::Isolate *isolate, rapidjson::Value::Array document) {
  int len = document.Size();
  v8::Local<v8::Array> result = v8::Array::New(isolate, len);

  
  int i;
  for (i = 0; i < len; i++) {
    result->Set(i, ConvertDocument(isolate, document[i].GetObject()));
  }

  return result;
}

v8::Local<v8::Value> ConvertDocument(v8::Isolate *isolate, rapidjson::Value document) {
  if (document.IsObject()) {
    return ConvertObject(isolate, document.GetObject());
  } else if (document.IsArray()) {
    return ConvertArray(isolate, document.GetArray());
  } else if (document.IsString()) {
    return v8::String::NewFromOneByte(isolate, (uint8_t *)document.GetString());
  }
}

void JsonTaskDone(uv_work_t *req, int code) {
  v8::Isolate *isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  Work *work = static_cast<Work *>(req->data);
  rapidjson::Document *document = work->document;

  v8::Local<v8::Value> result = ConvertDocument(isolate, document->GetObject());

  v8::Handle<v8::Value> argv[] = { result };
  v8::Local<v8::Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  work->callback.Reset();

  free(work->input);
  delete work->document;
  delete work;
}

void Parse(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate *isolate = args.GetIsolate();

  v8::String::Utf8Value inputV8Str(args[0]->ToString());
  v8::Local<v8::Uint32> len = v8::Local<v8::Uint32>::Cast(args[1]);
  uint32_t lenInt = len->Value();

  char *cstr = (char *) malloc(lenInt + 1);
  memcpy(cstr, (void *) *inputV8Str, lenInt);
  cstr[lenInt] = 0;

  Work *work = new Work();
  work->input = cstr;
  work->length = lenInt;
  work->request.data = work;

  v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[2]);
  work->callback.Reset(isolate, callback);

  uv_queue_work(uv_default_loop(), &work->request, StartJsonTask, JsonTaskDone);

  args.GetReturnValue().Set(Undefined(isolate));
}

void Init(v8::Local<v8::Object> exports) {
  NODE_SET_METHOD(exports, "parse", Parse);
}

NODE_MODULE(addon, Init)

}
