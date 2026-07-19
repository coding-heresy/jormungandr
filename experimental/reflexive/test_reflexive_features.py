import os
import sys

try:
    import reflexive_features
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

def test_reflexive_features():
    ####################
    # simple lifetime test
    print(
        "==========   next line should be printed by C++   ==========",
        flush=True)
    test_lifetime = reflexive_features.TestLifetime()
    print("^^^^^^^^^^ previous line should be printed by C++ ^^^^^^^^^^",
          flush=True)
    print(
        "==========   next line should be printed by C++   ==========",
        flush=True)
    test_lifetime = None
    print("^^^^^^^^^^ previous line should be printed by C++ ^^^^^^^^^^",
          flush=True)
    
    ####################
    # test methods when class is default constructed

    test_class = reflexive_features.TestClass()
    print(
        "==========   next line should be printed by C++   ==========",
        flush=True)
    test_class.returns_void()
    print("^^^^^^^^^^ previous line should be printed by C++ ^^^^^^^^^^",
          flush=True)
    int_val = test_class.returns_int_const()
    print(f"int return value was [{int_val}]")
    dbl_val = test_class.returns_dbl_const()
    print(f"double return value was [{dbl_val}]")
    str_val = test_class.returns_str_const()
    print(f"string return value was [{str_val}]")

    int_val = test_class.returns_int_arg(19440606)
    print(f"int return value from argument was [{int_val}]")

    ####################
    # test methods when class is constructed with a value

    # integer value
    test_class = reflexive_features.TestClass(17760704)
    int_val = test_class.returns_int_val()
    print(f"int return value was [{int_val}]")

    # integer value
    test_class = reflexive_features.TestClass("foo")
    str_val = test_class.returns_str_val()
    print(f"string return value was [{str_val}]")

    ####################
    # test failures
    failer = reflexive_features.TestClass()
    try:
        # fails because failer wasn't constructed with an int
        int_val = failer.returns_int_val()
    except RuntimeError as e:
        err_msg = str(e)
        print(f"caught expected RuntimeError with message [{err_msg}]")

    try:
        # fails because of argument type mismatch
        int_val = failer.returns_int_arg("foo")
    except TypeError as e:
        err_msg = str(e)
        print(f"caught expected RuntimeError with message [{err_msg}]")

    ####################
    # test complete
    print("test of reflexive python features completed successfully")

if __name__ == "__main__":
    test_reflexive_features()
