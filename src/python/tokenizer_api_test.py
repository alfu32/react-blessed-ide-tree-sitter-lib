import unittest
from tokenizer_api import get_all_visible_tokens, get_languages, pm_get, pm_release


class TestTokenizer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.langs = get_languages()
        print("Available grammars:", cls.langs)

    # --- C language ---
    def test_c_tokens(self):
        pm = pm_get("c")
        code = """
        int main(int argc,const char **argv) {
            int x = 1;
            return x;
        }
        """
        tokens = get_all_visible_tokens(pm, code)
        self.assertTrue(len(tokens) > 10)
        found_main = any("main" in t["content"] for t in tokens)
        self.assertTrue(found_main)
        for t in tokens:
            print(t)
        pm_release(pm)

    # --- Python language ---
    def test_python_tokens(self):
        pm = pm_get("python")
        code = """
        def add(a, b):
            return a + b

        x = add(2, 3)
        """
        tokens = get_all_visible_tokens(pm, code)
        self.assertTrue(len(tokens) > 10)
        found_def = any("def" in t["content"] for t in tokens)
        found_add = any("add" in t["content"] for t in tokens)
        self.assertTrue(found_def and found_add)
        for t in tokens:
            print(t)

    # --- JavaScript language ---
    def test_javascript_tokens(self):
        pm = pm_get("javascript")
        code = """
        function sum(a, b) {
            return a + b;
        }

        let x = sum(1, 2);
        """
        tokens = get_all_visible_tokens(pm, code)
        self.assertTrue(len(tokens) > 10)
        found_func = any("function" in t["content"] for t in tokens)
        found_sum = any("sum" in t["content"] for t in tokens)
        self.assertTrue(found_func and found_sum)
        for t in tokens:
            print(t)

    # --- HTML language ---
    def test_html_tokens(self):
        pm = pm_get("html")
        code = """
        <html>
          <head><title>Example</title></head>
          <body><h1>Hello World</h1></body>
        </html>
        """
        tokens = get_all_visible_tokens(pm, code)
        self.assertTrue(len(tokens) > 10)
        found_tag = any("html" in t["content"] for t in tokens)
        found_title = any("title" in t["content"] for t in tokens)
        self.assertTrue(found_tag and found_title)
        for t in tokens:
            print(t)

    # --- JSON language ---
    def test_json_tokens(self):
        pm = pm_get("json")
        code = """
        {
          "name": "John",
          "age": 30,
          "active": true
        }
        """
        tokens = get_all_visible_tokens(pm, code)
        self.assertTrue(len(tokens) > 5)
        found_name = any("name" in t["content"] for t in tokens)
        found_true = any("true" in t["content"] for t in tokens)
        self.assertTrue(found_name and found_true)
        for t in tokens:
            print(t)

    # --- Window filtering test (C language) ---
    def test_window_filter(self):
        pm = pm_get("c")
        code = """
        int a = 0;
        int b = 1;
        int c = 2;
        """
        all_tokens = get_all_visible_tokens(pm, code)
        filtered = get_all_visible_tokens(pm, code, x0=0, y0=1, x1=80, y1=1)
        self.assertTrue(len(all_tokens) > len(filtered))
        for t in filtered:
            self.assertTrue(t["y"] == 1)
            print(t)

if __name__ == "__main__":
    unittest.main(verbosity=2)
    # tt=TestTokenizer()
    # TestTokenizer.setUpClass()
    # tt.test_c_tokens()
    # tt.test_python_tokens()
    # tt.test_javascript_tokens()
    # tt.test_html_tokens()
    # tt.test_json_tokens()
