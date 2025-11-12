from unittest import TestCase
from tokenizer_api import get_all_visible_tokens, get_languages


class TestTokenizer(TestCase):
    code_c = """
    int main(int argc,const char ** argc) {
        int x = 1;
        return x;
    }
    """
    def test_c(self):
        tokens = get_all_visible_tokens(self.code_c, "c")
        print("count:", len(tokens))
        for tok in tokens:
            print(tok.__str__())
    def test_get_languages(self):
        langs = get_languages()
        print("Available languages:", langs)



TestTokenizer().test_c()
TestTokenizer().test_get_languages()
