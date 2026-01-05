#include <stdio.h>
#include <json-c/json.h>

int main() {
    // Create a new JSON object
    struct json_object *json_obj = json_object_new_object();

    // Add a string key-value pair
    json_object_object_add(json_obj, "name", json_object_new_string("John Doe"));

    // Add an integer key-value pair
    json_object_object_add(json_obj, "age", json_object_new_int(30));

    // Add a boolean key-value pair
    json_object_object_add(json_obj, "is_student", json_object_new_boolean(0)); // 0 for false

    // Add an array
    struct json_object *json_array = json_object_new_array();
    json_object_array_add(json_array, json_object_new_string("C"));
    json_object_array_add(json_array, json_object_new_string("Python"));
    json_object_array_add(json_array, json_object_new_string("JavaScript"));
    json_object_object_add(json_obj, "languages", json_array);

    // Add a nested object
    struct json_object *nested_obj = json_object_new_object();
    json_object_object_add(nested_obj, "university", json_object_new_string("Example University"));
    json_object_object_add(nested_obj, "year", json_object_new_int(2023));
    json_object_object_add(json_obj, "education", nested_obj);

    // Print the JSON object as a string
    printf("Generated JSON:\n%s\n", json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PRETTY));

    // Free the memory allocated for the JSON object
    json_object_put(json_obj);

    return 0;
}
