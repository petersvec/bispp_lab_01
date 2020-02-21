#!/usr/bin/env python

from flask import Flask, g

import login
import index
import users
import transfer
import fcoinsjs
import fbankdb
from debug import catch_err

app = Flask(__name__)

app.add_url_rule("/", "index", index.index, methods=['GET', 'POST'])
app.add_url_rule("/users", "users", users.users)
app.add_url_rule("/transfer", "transfer", transfer.transfer, methods=['GET', 'POST'])
app.add_url_rule("/fcoinsjs", "fcoinsjs", fcoinsjs.fcoinsjs, methods=['GET'])
app.add_url_rule("/login", "login", login.login, methods=['GET', 'POST'])
app.add_url_rule("/logout", "logout", login.logout)

@app.after_request
@catch_err
def disable_xss_protection(response):
    response.headers.add("X-XSS-Protection", "0")
    return response

if __name__ == "__main__":
    app.run(host='0.0.0.0', port='80')
