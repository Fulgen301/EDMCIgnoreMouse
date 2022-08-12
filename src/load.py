import edmcignoremouse
from edmcignoremouse import plugin_start3
import inspect

def plugin_app(parent):
    return edmcignoremouse.plugin_app(inspect.currentframe().f_back.f_back.f_locals["self"].w.winfo_id())
