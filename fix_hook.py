path = r'c:\Users\ataka\Desktop\projelergenel\oldtclient\best+meow client\deney best client 1.6.1-win64\src\game\client\components\bestclient\menus_bestclient.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Fix: str_format without format args -> str_copy
old = 'str_format(aHookBuf, sizeof(aHookBuf), "Hook Amount: Off");'
new = 'str_copy(aHookBuf, "Hook Amount: Off", sizeof(aHookBuf));'
if old in content:
    content = content.replace(old, new)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print('Fixed: str_format -> str_copy')
else:
    idx = content.find('Hook Amount: Off')
    if idx >= 0:
        print('Context:', repr(content[idx-80:idx+100]))
    else:
        print('NOT FOUND at all')
