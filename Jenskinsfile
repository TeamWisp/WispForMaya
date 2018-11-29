pipeline {
    agent any
    stages {
		stage('Install'){
			steps{
				echo "Running ${env.BUILD_ID} on ${env.JENKINS_URL}"
				bat '''cd "%WORKSPACE%\\Scripts"
				call JenkinsWebhook.bat ":bulb: Building: %JOB_NAME%. Jenkins build nr: %BUILD_NUMBER%"
				cd "%WORKSPACE%
				rem install -remote "%WORKSPACE%" 
				if errorlevel 1 (
					cd "%WORKSPACE%\\Scripts" 
					JenkinsWebhook ":x: %JOB_NAME% Build Failed!! Jenskins build nr: %BUILD_NUMBER% - install failed"
					EXIT 1
				)'''
			}
        }
    }
}
