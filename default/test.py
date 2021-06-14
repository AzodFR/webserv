import cgi

print 'Content-type: text/html'
print

print '<html><head><title>Test Pyton</title></head><body>'
form = cgi.FieldStorage()
if form.getvalue('user') != None:
    print form.getvalue('user')

print '</body></html>'