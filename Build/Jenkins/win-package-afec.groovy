pipeline{
  agent{
    node{
      // this node should only run on
      label "Windows7_64bit"
    }
  }
  environment {
    BUILD_BRANCH = "${env.GIT_BRANCH}".replaceAll("origin/", "").replaceAll("/", "_")
    BUILD_TAG = "${env.GIT_COMMIT[0..7]}_${env.BUILD_BRANCH}"
  }
  triggers{
    // override the jobs trigger configuration 
    pollSCM('H 22 * * *') // check every night at 10PM for changes
  }
  stages{
    // clean previous builds
    stage('Cleanup'){
      steps{
        dir('Build/Out'){ 
          deleteDir() 
        }
        dir('Build'){
          fileOperations([fileDeleteOperation(includes: '*.zip', excludes: '')])
        }
      }
    }
    // build client tools
    stage('Build AFEC'){
      steps{
        dir('Build'){
          bat 'build.cmd'
          zip zipFile: "afec${BUILD_TAG}_win.zip", archive: true, dir: '../Dist/Release'
        }
      }
    }
  }
  post{
    success{
      emailext (
        subject: "Build succeeded: ${env.JOB_NAME} #${env.BUILD_NUMBER} - rev ${BUILD_TAG}",
        body: "Artifacts can be downloaded at:\n${env.BUILD_URL}artifact",
        recipientProviders: [[$class: 'DevelopersRecipientProvider'], [$class: 'RequesterRecipientProvider']]
      )
    }
    failure{
      emailext (
        subject: "Build failed: ${env.JOB_NAME} #${env.BUILD_NUMBER}",
        body: "Check console output for details at:\n${env.BUILD_URL}console",
        recipientProviders: [[$class: 'DevelopersRecipientProvider'], [$class: 'RequesterRecipientProvider']]
      )
    }
  }
}