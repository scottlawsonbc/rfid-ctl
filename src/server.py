import flask

app = flask.Flask(__name__)


@app.route('/start', methods=['POST'])
def start(): 
    data = flask.request.get_json()
    print('/start', data)
    return flask.jsonify({"success": True})


if __name__ == '__main__':
    app.run(debug=True)