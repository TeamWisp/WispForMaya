pipeline {
    agent any
	environment{
		MAYA_2018_DIR = 'C:/Maya-Devkit'
	}
    stages {
		stage('Install'){
			steps{
				echo "Running ${env.BUILD_ID} on ${env.JENKINS_URL}"
				bat '''cd "%WORKSPACE%\\Scripts"
				call JenkinsWebhook.bat ":bulb: Building: %JOB_NAME%. Jenkins build nr: %BUILD_NUMBER%"
				cd "%WORKSPACE%
				install -remote "%WORKSPACE%" 
				if errorlevel 1 (
					cd "%WORKSPACE%\\Scripts" 
					JenkinsWebhook ":x: %JOB_NAME% Build Failed!! Jenskins build nr: %BUILD_NUMBER% - install failed"
					EXIT 1
				)'''
			}
        }
		stage('Build'){
			steps{
				bat'''
				cd "%WORKSPACE%"
				cmake --build ./build_vs2017_win64 
				if errorlevel 1 (
					cd "%WORKSPACE%\\Scripts" 
					JenkinsWebhook ":x: %JOB_NAME% Build Failed!! Jenskins build nr: %BUILD_NUMBER% - 64bit-debug build failed"
					EXIT 1
				)
				'''

				bat'''
				cd "%WORKSPACE%"
				cmake --build ./build_vs2017_win64 --config Release
				if errorlevel 1 (
					cd "%WORKSPACE%\\Scripts" 
					JenkinsWebhook ":x: %JOB_NAME% Build Failed!! Jenskins build nr: %BUILD_NUMBER% - 64bit-release build failed"
					EXIT 1
				)
				'''
			}
		}
		stage('Test'){
			steps{
				script{
					def has_failed = false
					try{
						bat'''
						cd "%WORKSPACE%"
						cd build_vs2017_win64/bin/debug
						MayaUnitTest.exe
						if errorlevel 1 (
							EXIT 1
						)
						'''
					}
					catch( exc ){
						has_failed = true
						bat'''
						cd "%WORKSPACE%\\Scripts" 
							JenkinsWebhook ":rage: %JOB_NAME% build nr: %BUILD_NUMBER% - 64bit-debug Tests failed"
						'''
					}
					try{
						bat'''
						cd "%WORKSPACE%"
						cd build_vs2017_win64/bin/release
						MayaUnitTest.exe
						if errorlevel 1 (
							EXIT 1
						)
						'''
					}
					catch( exc ){
						has_failed = true
						bat'''
						cd "%WORKSPACE%\\Scripts" 
							JenkinsWebhook ":rage: %JOB_NAME% build nr: %BUILD_NUMBER% - 64bit-release Tests failed"
						'''
					}
					if(has_failed){
						bat'''
						cd "%WORKSPACE%\\Scripts" 
							JenkinsWebhook ":x: %JOB_NAME% Build Failed!! Jenskins build nr: %BUILD_NUMBER% - Tests Failed"
							EXIT 1
						'''
					}
				}
			}
		}
		stage('Finalize'){
			steps{
				bat '''
				rem mkdir builds
				rem move ./RayTracingLib/Debug "./builds/build_%BUILD_NUMBER%"
				cd "%WORKSPACE%\\scripts
				call "JenkinsWebhook.bat" ":white_check_mark: %JOB_NAME% Build Succesfull!! Jenkins build nr: %BUILD_NUMBER%"
				'''
			}
		}
    }
}
