#include <stdlib.h>
#include <assert.h>
#include "ezgrpc2_list.h"
#include "unity.h"

void setUp(void) {
}
void tearDown(void) {
}

void test_list_creation() {
  ezgrpc2_list *list = ezgrpc2_list_new(NULL);
  TEST_ASSERT_NOT_NULL(list);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_free(list);
}

void test_list_push_and_pop_back_1() {
  ezgrpc2_list *list = ezgrpc2_list_new(NULL);
  TEST_ASSERT_NOT_NULL(list);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_push_back(list, "Alpha");
  TEST_ASSERT_EQUAL_UINT64(1, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_EQUAL_MEMORY("Alpha", ezgrpc2_list_pop_back(list), 6);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_free(list);
}

void test_list_push_and_pop_back_2() {
  ezgrpc2_list *list = ezgrpc2_list_new(NULL);
  TEST_ASSERT_NOT_NULL(list);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_push_back(list, "Alpha");
  TEST_ASSERT_EQUAL_UINT64(1, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_push_back(list, "Beta");
  TEST_ASSERT_EQUAL_UINT64(2, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_EQUAL_MEMORY("Beta", ezgrpc2_list_pop_back(list), 5);
  TEST_ASSERT_EQUAL_UINT64(1, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_EQUAL_MEMORY("Alpha", ezgrpc2_list_pop_back(list), 6);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_free(list);
}

void test_list_push_and_pop_front_1() {
  ezgrpc2_list *list = ezgrpc2_list_new(NULL);
  TEST_ASSERT_NOT_NULL(list);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_push_front(list, "Alpha");
  TEST_ASSERT_EQUAL_UINT64(1, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_EQUAL_MEMORY("Alpha", ezgrpc2_list_pop_front(list), 6);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_free(list);
}

void test_list_push_and_pop_front_2() {
  ezgrpc2_list *list = ezgrpc2_list_new(NULL);
  TEST_ASSERT_NOT_NULL(list);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_push_front(list, "Alpha");
  TEST_ASSERT_EQUAL_UINT64(1, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_push_front(list, "Beta");
  TEST_ASSERT_EQUAL_UINT64(2, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_EQUAL_MEMORY("Beta", ezgrpc2_list_pop_front(list), 5);
  TEST_ASSERT_EQUAL_UINT64(1, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_EQUAL_MEMORY("Alpha", ezgrpc2_list_pop_front(list), 6);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  ezgrpc2_list_free(list);
}

void test_list_concat() {

}


void test_list_random() {
  ezgrpc2_list *list = ezgrpc2_list_new(NULL);
  TEST_ASSERT_NOT_NULL(list);
  TEST_ASSERT_NULL(ezgrpc2_list_pop_back(list));
  TEST_ASSERT_NULL(ezgrpc2_list_pop_front(list));

  ezgrpc2_list_push_front(list, "Alpha");
  TEST_ASSERT_EQUAL_MEMORY("Alpha", ezgrpc2_list_pop_back(list), 6);
  TEST_ASSERT_NULL(ezgrpc2_list_pop_back(list));
  TEST_ASSERT_NULL(ezgrpc2_list_pop_front(list));
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));

  ezgrpc2_list_push_back(list, "Beta");
  ezgrpc2_list_push_front(list, "Gamma");
  TEST_ASSERT_EQUAL_MEMORY("Beta", ezgrpc2_list_pop_back(list), 5);
  TEST_ASSERT_EQUAL_MEMORY("Gamma", ezgrpc2_list_pop_back(list), 6);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_NULL(ezgrpc2_list_pop_back(list));
  TEST_ASSERT_NULL(ezgrpc2_list_pop_front(list));
  ezgrpc2_list_push_back(list, "Alpha");
  ezgrpc2_list_push_back(list, "Beta");
  ezgrpc2_list_push_back(list, "Gamma");


  TEST_ASSERT_EQUAL_UINT64(3, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_EQUAL_MEMORY("Alpha", ezgrpc2_list_pop_front(list), 6);
  TEST_ASSERT_EQUAL_MEMORY("Beta", ezgrpc2_list_pop_front(list), 5);
  TEST_ASSERT_EQUAL_MEMORY("Gamma", ezgrpc2_list_pop_front(list), 6);
  TEST_ASSERT_EQUAL_UINT64(0, (uint64_t)ezgrpc2_list_count(list));
  TEST_ASSERT_NULL(ezgrpc2_list_pop_back(list));
  TEST_ASSERT_NULL(ezgrpc2_list_pop_front(list));

  ezgrpc2_list_free(list);

}
void foreach(const void *data, const void *userdata) {
  puts((char*)data);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_list_creation);
  RUN_TEST(test_list_push_and_pop_back_1);
  RUN_TEST(test_list_push_and_pop_back_2);
  RUN_TEST(test_list_push_and_pop_front_1);
  RUN_TEST(test_list_push_and_pop_front_2);
  RUN_TEST(test_list_random);
  return UNITY_END();
}
