import os
import sys

try:
    import reflexive_features
    from reflexive_features import (
        TestClass,
        TestLifetime,
        TestContainer,
    )
except UnicodeDecodeError as e:
    print("\n--- BAZEL PYTHON IMPORT CRASH DIAGNOSTIC ---", file=sys.stderr)
    print(f"Error Type: {type(e).__name__}", file=sys.stderr)
    print(f"Error Message: {e}", file=sys.stderr)
    print(f"Failed Encoding: {e.encoding}", file=sys.stderr)
    # The 'object' attribute contains the exact bytes Python tried to decode
    print(f"The exact offending bytes: {e.object}", file=sys.stderr)
    print(f"Crash position index: {e.start}", file=sys.stderr)

    print("\n--- RELEVANT BAZEL ENV VARIABLES ---", file=sys.stderr)
    for key in ['LANG', 'LC_ALL', 'PYTHONIOENCODING', 'BAZEL_TEST']:
        print(f"{key}: {os.environ.get(key, 'NOT SET')}", file=sys.stderr)

    # Force exit so Bazel registers the failure cleanly
    sys.exit(1)


# flush after every print to make debugging easier
def printfl(str):
    print(str, flush=True)


def test_reflexive_features():
    ####################
    # simple lifetime test
    printfl("==========   next line should be printed by C++   ==========")
    test_lifetime = TestLifetime()
    printfl("^^^^^^^^^^ previous line should be printed by C++ ^^^^^^^^^^")
    printfl("==========   next line should be printed by C++   ==========")
    test_lifetime = None
    printfl("^^^^^^^^^^ previous line should be printed by C++ ^^^^^^^^^^")

    ####################
    # test methods when class is default constructed

    printfl("-----------------------------------------------------------------------------")

    test_class = TestClass()
    printfl(
        "==========   next line should be printed by C++   ==========")
    test_class.returns_void()
    printfl("^^^^^^^^^^ previous line should be printed by C++ ^^^^^^^^^^")
    int_val = test_class.returns_int_const()
    printfl(f"int return value was [{int_val}]")
    dbl_val = test_class.returns_dbl_const()
    printfl(f"double return value was [{dbl_val}]")
    str_val = test_class.returns_str_const()
    printfl(f"string return value was [{str_val}]")

    # call methods using positional arguments

    int_val = test_class.returns_int_arg(19440606)
    printfl(f"int return value from argument was [{int_val}]")

    str_val = test_class.returns_std_string_arg("foo")
    printfl(f"string return value from argument was [{str_val}]")

    str_val = test_class.returns_std_string_view_arg("bar")
    printfl(f"string return value from string_view argument was [{str_val}]")

    int_val = test_class. add_and_set(1, 3)
    printfl(f"int return value from adding and setting was [{int_val}]")

    int_val = test_class. add_and_set_with_default(2, 4)
    printfl(f"int return value from defaulting adding and setting was [{int_val}]")

    # use the default parameter
    int_val = test_class. add_and_set_with_default(2)
    printfl(f"int return value from defaulting adding and setting was [{int_val}]")

    # call methods using keyword arguments

    int_val = test_class.returns_int_arg(arg=19450508)
    printfl(f"int return value from argument was [{int_val}]")

    str_val = test_class.returns_std_string_arg(str="goo")
    printfl(f"string return value from argument was [{str_val}]")

    str_val = test_class.returns_std_string_view_arg(str="car")
    printfl(f"string return value from argument was [{str_val}]")

    # pass 1 positional, 1 keyword
    int_val = test_class.add_and_set(3, arg2=5)
    printfl(f"int return value from adding was [{int_val}]")

    # pass both keywords, in parameter order
    int_val = test_class.add_and_set(arg1=3, arg2=5)
    printfl(f"int return value from adding was [{int_val}]")

    # pass both keywords, in reverse of parameter order
    int_val = test_class.add_and_set(arg2=7, arg1=5)
    printfl(f"int return value from adding was [{int_val}]")

    # print the stored value to ensure that it was set by the correct
    # keyword argument
    int_val = test_class.returns_int_val()
    printfl(f"int return value from previous set was [{int_val}]")

    ####################
    # overloaded methods that return one argument

    int_val = test_class.overloaded_arg_return(-1)
    printfl(f"int return value from overloaded function [{int_val}]")

    str_val = test_class.overloaded_arg_return("heavily")
    printfl(f"string return value from overloaded function [{str_val}]")

    ####################
    # static methods that return one argument

    # call methods using positional arguments

    int_val = TestClass.static_returns_int_arg(19411207)
    printfl(f"int return value from argument was [{int_val}]")

    str_val = TestClass.static_returns_std_string_arg("FOO")
    printfl(f"string return value from argument was [{str_val}]")

    str_val = TestClass.static_returns_std_string_view_arg("BAR")
    printfl(f"string return value from argument was [{str_val}]")

    # call methods using keyword arguments

    str_val = TestClass.static_returns_cat_args("the answer is: ", int_arg=42)
    printfl(f"string return value from argument concatenation was [{str_val}]")

    str_val = TestClass.static_returns_cat_args(int_arg=91, str_arg="now in '")
    printfl(f"string return value from argument concatenation was [{str_val}]")

    ####################
    # test methods when class is constructed with a value

    # call constructors using positional arguments

    # integer value
    test_class = TestClass(17760704)
    int_val = test_class.returns_int_val()
    printfl(f"int return value was [{int_val}]")

    # string value
    test_class = TestClass("blub")
    str_val = test_class.returns_str_val()
    printfl(f"string return value from internal data was [{str_val}]")

    ####################
    # test construction with keyword arguments

    test_class = TestClass(int_val=19760704)
    int_val = test_class.returns_int_val()
    printfl(f"int return value was [{int_val}]")

    test_class = TestClass(str_val="blab")
    str_val = test_class.returns_str_val()
    printfl(f"string return value from internal data was [{str_val}]")

    ####################
    # test data members as attributes

    # getting values
    int_val = test_class.int_data_member
    printfl(f"value of int_data_member was [{int_val}]")

    str_val = test_class.str_data_member
    printfl(f"value of str_data_member was [{str_val}]")

    str_val = test_class.str_view_data_member
    printfl(f"value of str_view_data_member was [{str_val}]")

    # setting values
    test_class.int_data_member = 19700101
    int_val = test_class.int_data_member
    printfl(f"value of int_data_member was updated to [{int_val}]")

    test_class.str_data_member = "too close"
    str_val = test_class.str_data_member
    printfl(f"value of str_data_member was updated to [{str_val}]")

    ####################
    # test string representation

    printfl(f"string representation >>>>> {test_class} <<<<<")

    ####################
    # test static data members as class attributes

    # getting values
    int_val = TestClass.static_int_data_member
    printfl(f"value of static_int_data_member was [{int_val}]")

    str_val = TestClass.static_str_data_member
    printfl(f"value of static_str_data_member was [{str_val}]")

    str_val = TestClass.static_str_view_data_member
    printfl(f"value of static_str_view_data_member was [{str_val}]")

    # setting values
    TestClass.static_int_data_member = 20260101
    int_val = TestClass.static_int_data_member
    printfl(f"value of static_int_data_member was updated to [{int_val}]")

    TestClass.static_str_data_member = "never too close"
    str_val = TestClass.static_str_data_member
    printfl(f"value of static_str_data_member was updated to [{str_val}]")

    ####################
    # test container

    test_container = TestContainer()
    printfl(f"length of container is '{len(test_container)}'")
    printfl(f"string representation of container is {test_container}")
    for item in test_container:
        printfl(f"--> container item is [{item}]")
    printfl(f"item 2 is [{test_container[2]}]")
    test_container[2] = test_container[2] * 2
    printfl("after modifying item 2:")
    for item in test_container:
        printfl(f"--> container item is [{item}]")
    printfl(f"last container item value is [{test_container[-1]}]")

    ####################
    # test failures

    printfl("-----------------------------------------------------------------------------")

    failer = TestClass()
    try:
        # fails because failer wasn't constructed with an int
        int_val = failer.returns_int_val()
    except RuntimeError as e:
        err_msg = str(e)
        printfl(f"caught expected RuntimeError with message [{err_msg}]")

    try:
        # fails because of argument type mismatch
        int_val = failer.returns_int_arg("foo")
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    try:
        # fails because an argument is required
        int_val = test_class.returns_int_arg()
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    try:
        # fails because not all arguments are provided
        int_val = test_class.add_and_set(1)
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    try:
        # fails because the keyword did not match a c++ parameter
        int_val = test_class.returns_int_arg(val=0)
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    try:
        # fails because of a missing keyword argument
        int_val = test_class.add_and_set(arg2=0)
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    # TODO(bd) fail setting const data members

    # TODO(bd) fail passing arguments that don't match parameters:
    # - positional argument to no-argument function
    # - more positional arguments than needed by function
    # - keyword argument that doesn't match parameter

    try:
        # fails because object is not iterable
        for item in failer:
            printfl(item)
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    try:
        # fails because object does not expose a length
        printfl(f"{len(failer)}")
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    try:
        # fails because object is not subscriptable
        printfl(f"{failer[0]}")
    except TypeError as e:
        err_msg = str(e)
        printfl(f"caught expected TypeError with message [{err_msg}]")

    ####################
    # test complete
    printfl("test of reflexive python features completed successfully")

if __name__ == "__main__":
    test_reflexive_features()
