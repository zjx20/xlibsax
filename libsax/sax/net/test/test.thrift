
namespace cpp test

struct TestReq {
  1: string req
}

struct TestRes {
  1: string res
}

service Test {
  TestRes test(1:TestReq req)
}