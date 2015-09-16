   // Rigid body for the bowling ball.
   currentPos = bowlingBallRigidBody->getCenterOfMassPosition();
   if(currentPos.y() < 2.1)
      {
      // Once bowling ball has reached the ground, allow user to choose 
      // bowling ball attributes.
      disableKeyboard = false; 
      chooseBowlingBallAttributes(dt, currentPos, projectionAngle, powerAngle);
      //cout << "Passed back angle: " << projectionAngle << endl;  
      }

   if(startPositionFound == true &&
      startProjectionAngleFound == true && 
      startPowerFound == true &&
      test == false) 
      {
      cout << "XZ plane angle: " << projectionAngle << endl; 
      bool isSlopePositive = true;
      if(projectionAngle < 0)
         {
         cout << "Slope is positive" << endl; 
         isSlopePositive = true; 
         }
      else
         {
         cout << "Slope is not positive" << endl; 
         isSlopePositive = false; 
         }

      cout << "Current X Position: " << currentPos.x() << endl; 
      static float angle = 90.0 - abs(projectionAngle);
      cout << "Tangent angle: " << angle << endl;

      // Determine length of "opposite" side of triangle.
      // Note: We are converting to radians because that's what "tan"
      // accepts as an argument. 
      static float opposite = currentPos.x() * tan(degreesToRadians(angle));


/*
      cout << "Opposite: " << opposite << endl; 
      static float slope = abs(opposite) / float(currentPos.x());
      if(isSlopePositive == true)
         {
         slope = abs(slope);
         for(float i = currentPos.x(); i < 0; i += 0.1)
            {
            cout << "x: " << i << endl;
            cout << "slope times x: " << slope * i << endl;  
            float newZ = abs((slope * i)) + opposite;
            cout << "New Z: " << newZ << endl;  
            bowlingBallRigidBody->translate(btVector3(i, 0, newZ));
            bowlingBallRigidBody->getMotionState()->getWorldTransform(trans);
            trans.getOpenGLMatrix(m);
            model[BOWLING_BALL] = glm::make_mat4(m);
            }
         } 
      else if(isSlopePositive == false)
         {
         slope = -1 * abs(slope); 
         }


      cout << "Slope: " << slope << endl;
*/
      btVector3 newBall = bowlingBallRigidBody->getCenterOfMassPosition();
      newBall[0] = 0.05;
      newBall[2] = -0.05;
      test = true;
      bowlingBallRigidBody->translate(newBall);
      disableKeyboard = true;
      //exit(EXIT_FAILURE);
      }





// Rotate up and down
      // Determine which way to rotate our pointer.
      if(radiansToDegrees(rotAngle > 0)
         {
         rotateUp = false;  
         }
      else if(rotAngle < -1)
         {
         rotateUp = true;
         }

      // Rotate pointer up. 
      if(rotateUp == true)
         {
         rotAngle += (currentTime * M_PI/2) / 4;
         }

      // Rotate pointer down. 
      else if(rotateUp == false)
         {
         rotAngle -= (currentTime * M_PI/2) / 4;
         }

      // Rotate the pointer through degrees of projection. 
      glm::mat4 pointerRot = glm::rotate(finalYAxisRotation,
                                          rotAngle,
                                          glm::vec3(1.0, 0.0, 0.0));
      model[POINTER] = pointerTrans * pointerRot;
      yzPlaneAngle = rotAngle;